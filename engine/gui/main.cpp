#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QLockFile>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "game.hpp"
#include "history.hpp"

class ChessWindow : public QWidget {
public:
    ChessWindow() {
        setWindowTitle("Chess");

        assets_ = QDir(QString::fromUtf8(CHESS_ASSET_PATH));
        save_path_ = QString::fromUtf8(CHESS_HISTORY_PATH);

        const QString data_directory =
            QFileInfo(save_path_).absolutePath();

        if (!QDir().mkpath(data_directory)) {
            throw std::runtime_error(
                "Could not create the history directory.");
        }

        // Prevent two GUI windows from allocating the same local ID.
        history_lock_ =
            std::make_unique<QLockFile>(save_path_ + ".lock");

        if (!history_lock_->tryLock(0)) {
            throw std::runtime_error(
                "Could not lock local history. "
                "Another Chess window may already be open.");
        }

        auto* layout = new QVBoxLayout(this);

        status_ = new QLabel(this);
        status_->setWordWrap(true);
        layout->addWidget(status_);

        auto* board_layout = new QGridLayout;
        board_layout->setSpacing(0);
        layout->addLayout(board_layout);

        for (int row = 0; row < 8; ++row) {
            ranks_[row] = new QLabel(this);
            board_layout->addWidget(
                ranks_[row], row, 0, Qt::AlignCenter);

            for (int col = 0; col < 8; ++col) {
                auto* square = new QPushButton(this);
                square->setFixedSize(64, 64);
                square->setIconSize(QSize(52, 52));
                square->setFocusPolicy(Qt::NoFocus);

                squares_[row][col] = square;
                board_layout->addWidget(
                    square, row, col + 1);

                connect(square, &QPushButton::clicked,
                        this, [this, row, col] {
                    const bool flipped = board_flipped();
                    click_square(
                        flipped ? 7 - row : row,
                        flipped ? 7 - col : col);
                });
            }
        }

        for (int col = 0; col < 8; ++col) {
            files_[col] = new QLabel(this);
            board_layout->addWidget(
                files_[col], 8, col + 1, Qt::AlignCenter);
        }

        menu_panel_ = new QWidget(this);
        auto* menu_layout = new QHBoxLayout(menu_panel_);
        menu_layout->setContentsMargins(0, 0, 0, 0);

        auto* local = new QPushButton("Local", this);
        auto* random = new QPushButton("Random", this);
        auto* ai = new QPushButton("AI", this);
        auto* replay = new QPushButton("Replay", this);

        ai->setEnabled(false);
        ai->setToolTip("Coming later");

        menu_layout->addWidget(local);
        menu_layout->addWidget(random);
        menu_layout->addWidget(ai);
        menu_layout->addWidget(replay);
        layout->addWidget(menu_panel_);

        connect(local, &QPushButton::clicked,
                this, [this] {
            start_game(Mode::Local, true);
        });

        connect(random, &QPushButton::clicked,
                this, [this] {
            bool confirmed = false;
            const QStringList choices{"White", "Black"};

            const QString choice = QInputDialog::getItem(
                this, "Random opponent", "Play as:",
                choices, 0, false, &confirmed);

            if (confirmed) {
                start_game(
                    Mode::Random, choice == "White");
            }
        });

        connect(replay, &QPushButton::clicked,
                this, [this] {
            open_history();
        });

        replay_panel_ = new QWidget(this);
        auto* replay_layout = new QVBoxLayout(replay_panel_);
        replay_layout->setContentsMargins(0, 0, 0, 0);

        // Choose which history file Replay reads.
        auto* source_layout = new QHBoxLayout;
        source_layout->addWidget(
            new QLabel("Replay source:", this));

        replay_source_ = new QComboBox(this);
        replay_source_->addItem("Local games");
        replay_source_->addItem("Random games");
        source_layout->addWidget(replay_source_, 1);
        replay_layout->addLayout(source_layout);

        auto* selection_layout = new QHBoxLayout;

        games_box_ = new QComboBox(this);
        games_box_->setMaxVisibleItems(10);
        games_box_->setMinimumWidth(240);
        games_box_->setStyleSheet(
            "QComboBox { combobox-popup: 0; }");

        id_box_ = new QLineEdit(this);
        id_box_->setPlaceholderText("Game ID");
        id_box_->setMaximumWidth(140);

        auto* load = new QPushButton("Load", this);

        selection_layout->addWidget(games_box_, 1);
        selection_layout->addWidget(id_box_);
        selection_layout->addWidget(load);
        replay_layout->addLayout(selection_layout);

        auto* navigation = new QHBoxLayout;
        previous_ = new QPushButton("Previous", this);
        next_ = new QPushButton("Next", this);
        replay_progress_ = new QLabel(this);
        replay_progress_->setAlignment(Qt::AlignCenter);

        navigation->addWidget(previous_);
        navigation->addWidget(replay_progress_, 1);
        navigation->addWidget(next_);
        replay_layout->addLayout(navigation);

        layout->addWidget(replay_panel_);

        connect(replay_source_,
                &QComboBox::currentIndexChanged,
                this, [this](int) {
            if (mode_ == Mode::Replay) {
                select_replay_source();
            }
        });

        connect(games_box_,
                &QComboBox::currentIndexChanged,
                this, [this](int index) {
            if (index >= 0 &&
                index < static_cast<int>(records_.size())) {
                load_replay(records_[index]);
            }
        });

        connect(load, &QPushButton::clicked,
                this, [this] {
            load_typed_id();
        });

        connect(id_box_, &QLineEdit::returnPressed,
                this, [this] {
            load_typed_id();
        });

        connect(previous_, &QPushButton::clicked,
                this, [this] {
            if (replay_position_ > 0) {
                show_replay_position(
                    replay_position_ - 1);
            }
        });

        connect(next_, &QPushButton::clicked,
                this, [this] {
            if (replay_position_ < replay_moves_.size()) {
                show_replay_position(
                    replay_position_ + 1);
            }
        });

        menu_button_ = new QPushButton("Menu", this);
        layout->addWidget(menu_button_);

        bot_timer_ = new QTimer(this);
        bot_timer_->setSingleShot(true);

        connect(bot_timer_, &QTimer::timeout,
                this, [this] {
            play_random_move();
        });

        connect(menu_button_, &QPushButton::clicked,
                this, [this] {
            menu_open_ = !menu_open_;
            bot_timer_->stop();
            clear_selection();
            refresh();

            if (!menu_open_) {
                schedule_bot();
            }
        });

        refresh();
    }

private:
    enum class Mode {
        None,
        Local,
        Random,
        Replay
    };

    Game game_;
    Game replay_game_;

    Mode mode_ = Mode::None;
    bool menu_open_ = true;
    bool human_white_ = true;
    bool started_ = false;

    QDir assets_;
    QString save_path_;
    std::unique_ptr<QLockFile> history_lock_;

    std::mt19937 random_{std::random_device{}()};
    QTimer* bot_timer_ = nullptr;

    QLabel* status_ = nullptr;
    QWidget* menu_panel_ = nullptr;
    QWidget* replay_panel_ = nullptr;
    QPushButton* menu_button_ = nullptr;

    std::array<QLabel*, 8> ranks_{};
    std::array<QLabel*, 8> files_{};
    std::array<std::array<QPushButton*, 8>, 8> squares_{};

    int selected_row_ = -1;
    int selected_col_ = -1;
    std::vector<GameMove> legal_moves_;

    QComboBox* replay_source_ = nullptr;
    QComboBox* games_box_ = nullptr;
    QLineEdit* id_box_ = nullptr;
    QPushButton* previous_ = nullptr;
    QPushButton* next_ = nullptr;
    QLabel* replay_progress_ = nullptr;

    std::vector<history::Record> records_;
    std::vector<GameMove> replay_moves_;
    history::Record replay_record_;
    std::size_t replay_position_ = 0;
    bool replay_loaded_ = false;

    bool live_mode() const {
        return mode_ == Mode::Local ||
               mode_ == Mode::Random;
    }

    bool board_flipped() const {
        if (mode_ == Mode::Local) {
            return !game_.white_to_move();
        }

        if (mode_ == Mode::Random) {
            return !human_white_;
        }

        return false;
    }

    bool human_can_move() const {
        return !menu_open_ &&
            live_mode() &&
            !game_.finished() &&
            (mode_ == Mode::Local ||
             game_.white_to_move() == human_white_);
    }

    void clear_selection() {
        selected_row_ = -1;
        selected_col_ = -1;
        legal_moves_.clear();
    }

    bool confirm_discard() {
        if (!live_mode() ||
            !started_ ||
            game_.finished()) {
            return true;
        }

        return QMessageBox::question(
            this, "Leave current game",
            "Discard the unfinished game?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No) == QMessageBox::Yes;
    }

    void start_game(
        Mode mode, bool human_white) {
        if (!confirm_discard()) {
            return;
        }

        bot_timer_->stop();
        game_ = Game{};
        mode_ = mode;
        human_white_ = human_white;
        started_ = false;
        menu_open_ = false;

        clear_selection();
        refresh();
        schedule_bot();
    }

    QString result_text() const {
        if (game_.reason() == "checkmate") {
            return game_.result() == "1-0"
                ? "White wins by checkmate!"
                : "Black wins by checkmate!";
        }

        return QString("Draw: %1.")
            .arg(QString::fromStdString(
                game_.reason()));
    }

    void accept_move(const GameMove& move) {
        if (!game_.play(move)) {
            clear_selection();
            refresh();
            return;
        }

        started_ = true;
        clear_selection();
        refresh();

        if (game_.finished()) {
            bot_timer_->stop();

            const bool saved =
                game_.save_finished_game(
                    save_path_.toStdString());

            QMessageBox dialog(this);
            dialog.setWindowTitle("Game over");
            dialog.setTextFormat(Qt::PlainText);
            dialog.setIcon(saved
                ? QMessageBox::Information
                : QMessageBox::Warning);
            dialog.setText(result_text());

            dialog.setInformativeText(
                (saved
                    ? QString("Game saved to:\n%1")
                        .arg(save_path_)
                    : QString("Could not save the game to:\n%1")
                        .arg(save_path_)) +
                "\n\nUse Menu to choose another game.");

            dialog.exec();
        } else {
            schedule_bot();
        }
    }

    void click_square(int row, int col) {
        if (!human_can_move()) {
            return;
        }

        std::vector<GameMove> choices;

        for (const auto& move : legal_moves_) {
            if (move.to_row == row &&
                move.to_col == col) {
                choices.push_back(move);
            }
        }

        if (!choices.empty()) {
            GameMove move = choices.front();

            if (move.promotion != '\0') {
                bool confirmed = false;

                const QStringList labels{
                    "Queen", "Rook",
                    "Bishop", "Knight"
                };

                const QString choice =
                    QInputDialog::getItem(
                        this,
                        "Pawn promotion",
                        "Promote to:",
                        labels, 0,
                        false, &confirmed);

                const int index =
                    labels.indexOf(choice);

                if (!confirmed || index < 0) {
                    return;
                }

                move.promotion =
                    "qrbn"[index];
            }

            accept_move(move);
            return;
        }

        const bool clicked_selection =
            row == selected_row_ &&
            col == selected_col_;

        clear_selection();

        const Board& board = game_.board();
        const Piece* piece =
            board.get_piece(row, col, board);

        if (!clicked_selection &&
            piece &&
            piece->is_white ==
                game_.white_to_move()) {
            selected_row_ = row;
            selected_col_ = col;
            legal_moves_ =
                game_.legal_moves_from(row, col);
        }

        refresh();
    }

    void schedule_bot() {
        if (!menu_open_ &&
            mode_ == Mode::Random &&
            !game_.finished() &&
            game_.white_to_move() != human_white_) {
            bot_timer_->start(250);
        }
    }

    void play_random_move() {
        if (menu_open_ ||
            mode_ != Mode::Random ||
            game_.finished() ||
            game_.white_to_move() == human_white_) {
            return;
        }

        std::vector<GameMove> moves;

        for (int row = 0; row < 8; ++row) {
            for (int col = 0; col < 8; ++col) {
                auto from =
                    game_.legal_moves_from(row, col);

                moves.insert(
                    moves.end(),
                    from.begin(),
                    from.end());
            }
        }

        if (moves.empty()) {
            return;
        }

        std::uniform_int_distribution<std::size_t>
            pick(0, moves.size() - 1);

        accept_move(moves[pick(random_)]);
    }

    static GameMove parse_move(
        const std::string& text) {
        if ((text.size() != 4 &&
             text.size() != 5) ||
            text[0] < 'a' ||
            text[0] > 'h' ||
            text[1] < '1' ||
            text[1] > '8' ||
            text[2] < 'a' ||
            text[2] > 'h' ||
            text[3] < '1' ||
            text[3] > '8') {
            throw std::runtime_error(
                "Invalid move: " + text);
        }

        char promotion = '\0';

        if (text.size() == 5) {
            promotion = text[4];

            if (std::string("qrbn").find(
                    promotion) ==
                std::string::npos) {
                throw std::runtime_error(
                    "Invalid promotion: " + text);
            }
        }

        return {
            '8' - text[1],
            text[0] - 'a',
            '8' - text[3],
            text[2] - 'a',
            promotion
        };
    }

    static std::string placement(
        const Board& board) {
        std::string result;

        for (int row = 0; row < 8; ++row) {
            if (row != 0) {
                result += '/';
            }

            int empty = 0;

            for (int col = 0; col < 8; ++col) {
                const Piece* piece =
                    board.get_piece(
                        row, col, board);

                if (!piece) {
                    ++empty;
                    continue;
                }

                if (empty != 0) {
                    result +=
                        static_cast<char>(
                            '0' + empty);
                    empty = 0;
                }

                char letter =
                    piece->name == "Knight"
                        ? 'N'
                        : piece->name.front();

                if (!piece->is_white) {
                    letter =
                        static_cast<char>(
                            letter - 'A' + 'a');
                }

                result += letter;
            }

            if (empty != 0) {
                result +=
                    static_cast<char>(
                        '0' + empty);
            }
        }

        return result;
    }

    QString random_history_path() const {
        return QDir(
            QFileInfo(save_path_).absolutePath())
            .filePath("random_games.tsv");
    }

    void open_history() {
        if (!confirm_discard()) {
            return;
        }

        bot_timer_->stop();
        mode_ = Mode::Replay;
        menu_open_ = false;
        started_ = false;
        clear_selection();

        {
            const QSignalBlocker blocker(
                replay_source_);
            replay_source_->setCurrentIndex(0);
        }

        select_replay_source();
    }

    void select_replay_source() {
        const bool random_source =
            replay_source_->currentIndex() == 1;

        replay_loaded_ = false;
        replay_moves_.clear();
        replay_position_ = 0;
        replay_game_ = Game{};
        records_.clear();
        id_box_->clear();

        games_box_->setVisible(
            !random_source);

        id_box_->setPlaceholderText(
            random_source
                ? "Random game ID"
                : "Local game ID");

        {
            const QSignalBlocker blocker(
                games_box_);
            games_box_->clear();
        }

        if (random_source) {
            refresh();
            id_box_->setFocus();
            return;
        }

        try {
            history::Data data =
                history::load_and_migrate(
                    save_path_.toStdString());

            records_ =
                std::move(data.records);

            std::sort(
                records_.begin(),
                records_.end(),
                [](const auto& left,
                   const auto& right) {
                    return left.id >
                           right.id;
                });

            {
                const QSignalBlocker blocker(
                    games_box_);

                for (const auto& record :
                     records_) {
                    games_box_->addItem(
                        QString(
                            "Game %1 — %2 — %3")
                            .arg(
                                QString::number(
                                    static_cast<
                                        qulonglong>(
                                        record.id)))
                            .arg(
                                QString::fromStdString(
                                    record.result))
                            .arg(
                                QString::fromStdString(
                                    record.reason)));
                }

                games_box_->setCurrentIndex(
                    records_.empty()
                        ? -1 : 0);
            }

            if (!records_.empty()) {
                load_replay(
                    records_.front());
            } else {
                refresh();
            }
        } catch (
            const std::exception& error) {
            refresh();
            QMessageBox::warning(
                this,
                "History error",
                QString::fromUtf8(
                    error.what()));
        }
    }

    void load_typed_id() {
        std::uint64_t requested_id = 0;

        try {
            requested_id =
                history::parse_id(
                    id_box_->text()
                        .trimmed()
                        .toStdString());
        } catch (
            const std::exception& error) {
            QMessageBox::warning(
                this,
                "Invalid game ID",
                QString::fromUtf8(
                    error.what()));
            return;
        }

        if (replay_source_->currentIndex() == 0) {
            const auto found =
                std::find_if(
                    records_.begin(),
                    records_.end(),
                    [requested_id](
                        const auto& record) {
                        return record.id ==
                               requested_id;
                    });

            if (found ==
                records_.end()) {
                QMessageBox::information(
                    this,
                    "Game not found",
                    "There is no local game "
                    "with that ID.");
                return;
            }

            {
                const QSignalBlocker blocker(
                    games_box_);
                games_box_->setCurrentIndex(
                    static_cast<int>(
                        found -
                        records_.begin()));
            }

            load_replay(*found);
            return;
        }

        const QString path =
            random_history_path();

        std::ifstream input(
            path.toStdString());

        if (!input) {
            QMessageBox::information(
                this,
                "Random games not found",
                QString(
                    "Could not open:\n%1\n\n"
                    "Generate games with "
                    "chess_batch first.")
                    .arg(path));
            return;
        }

        std::string line;

        if (!std::getline(input, line)) {
            QMessageBox::warning(
                this,
                "History error",
                "The random games file "
                "is empty.");
            return;
        }

        if (!line.empty() &&
            line.back() == '\r') {
            line.pop_back();
        }

        if (line != history::new_header) {
            QMessageBox::warning(
                this,
                "History error",
                "The random games file has "
                "an unexpected header.");
            return;
        }

        while (
            std::getline(input, line)) {
            if (!line.empty() &&
                line.back() == '\r') {
                line.pop_back();
            }

            const auto fields =
                history::split_tabs(line);

            if (fields.size() != 5) {
                continue;
            }

            std::uint64_t row_id = 0;

            try {
                row_id =
                    history::parse_id(
                        fields[0]);
            } catch (
                const std::exception&) {
                continue;
            }

            if (row_id !=
                requested_id) {
                continue;
            }

            history::Record record;
            record.id = row_id;
            record.result = fields[1];
            record.reason = fields[2];
            record.moves = fields[3];
            record.final_placement =
                fields[4];

            load_replay(record);
            return;
        }

        if (input.bad()) {
            QMessageBox::warning(
                this,
                "History error",
                "Could not finish reading "
                "the random games file.");
        } else {
            QMessageBox::information(
                this,
                "Game not found",
                QString(
                    "Random game ID %1 "
                    "was not found.")
                    .arg(
                        QString::number(
                            static_cast<
                                qulonglong>(
                                requested_id))));
        }
    }

    void load_replay(
        const history::Record& record) {
        try {
            Game validation;
            std::vector<GameMove> moves;

            std::istringstream input(
                record.moves);
            std::string text;

            while (input >> text) {
                const GameMove move =
                    parse_move(text);

                if (!validation.play(
                        move)) {
                    throw
                        std::runtime_error(
                            "Illegal move " +
                            std::to_string(
                                moves.size() +
                                1) +
                            ": " + text);
                }

                moves.push_back(move);
            }

            if (placement(
                    validation.board()) !=
                record.final_placement) {
                throw std::runtime_error(
                    "The final board does "
                    "not match the saved "
                    "record.");
            }

            const bool
                valid_unfinished =
                    record.result ==
                        "unfinished" &&
                    record.reason ==
                        "move_limit" &&
                    !validation.finished();

            const bool
                valid_completed =
                    validation.finished() &&
                    validation.result() ==
                        record.result &&
                    validation.reason() ==
                        record.reason;

            if (!valid_unfinished &&
                !valid_completed) {
                throw std::runtime_error(
                    "The result does not "
                    "match the replayed "
                    "game.");
            }

            replay_record_ = record;
            replay_moves_ =
                std::move(moves);

            // Reset before showing the
            // first replay position.
            replay_game_ = Game{};
            replay_position_ = 0;
            replay_loaded_ = true;

            id_box_->setText(
                QString::number(
                    static_cast<
                        qulonglong>(
                        record.id)));

            show_replay_position(0);
        } catch (
            const std::exception& error) {
            replay_loaded_ = false;
            replay_moves_.clear();
            replay_position_ = 0;
            replay_game_ = Game{};
            refresh();

            QMessageBox::warning(
                this,
                "Cannot replay game",
                QString(
                    "Game %1:\n%2")
                    .arg(
                        QString::number(
                            static_cast<
                                qulonglong>(
                                record.id)))
                    .arg(
                        QString::fromUtf8(
                            error.what())));
        }
    }

    void show_replay_position(
        std::size_t position) {
        if (!replay_loaded_ ||
            position >
                replay_moves_.size()) {
            return;
        }

        // Rebuild from the start
        // when stepping backwards.
        if (position <
            replay_position_) {
            replay_game_ = Game{};
            replay_position_ = 0;
        }

        while (
            replay_position_ <
            position) {
            if (!replay_game_.play(
                    replay_moves_[
                        replay_position_])) {
                replay_loaded_ = false;
                refresh();

                QMessageBox::warning(
                    this,
                    "Replay error",
                    "Could not reconstruct "
                    "this position.");
                return;
            }

            ++replay_position_;
        }

        refresh();
    }

    void draw_piece(
        QPushButton* square,
        const Piece* piece) {
        square->setIcon(QIcon());
        square->setText(QString());

        if (!piece) {
            return;
        }

        const QString name =
            QString::fromStdString(
                piece->name)
                .toLower();

        const QString suffix =
            piece->is_white
                ? "-w.svg"
                : "-b.svg";

        const QIcon icon(
            assets_.filePath(
                name + suffix));

        if (icon.pixmap(
                square->iconSize())
                .isNull()) {
            square->setText(
                QString::fromStdString(
                    piece->symbol));
        } else {
            square->setIcon(icon);
        }
    }

    // Find every piece attacking the
    // checked king, including double check.
    std::vector<
        std::pair<int, int>>
    checking_pieces(
        const Board& board,
        bool checked_white,
        int king_row,
        int king_col) const {
        std::vector<
            std::pair<int, int>>
            attackers;

        for (int row = 0;
             row < 8; ++row) {
            for (int col = 0;
                 col < 8; ++col) {
                const Piece* piece =
                    board.get_piece(
                        row, col, board);

                if (!piece ||
                    piece->is_white ==
                        checked_white) {
                    continue;
                }

                const int dr =
                    king_row - row;
                const int dc =
                    king_col - col;

                const int distance_row =
                    std::abs(dr);
                const int distance_col =
                    std::abs(dc);

                bool attacks = false;

                if (dynamic_cast<
                        const Pawn*>(
                        piece)) {
                    const int direction =
                        piece->is_white
                            ? -1 : 1;

                    attacks =
                        dr == direction &&
                        distance_col == 1;
                } else if (
                    dynamic_cast<
                        const Knight*>(
                        piece)) {
                    attacks =
                        (distance_row == 2 &&
                         distance_col == 1) ||
                        (distance_row == 1 &&
                         distance_col == 2);
                } else if (
                    dynamic_cast<
                        const King*>(
                        piece)) {
                    attacks =
                        distance_row <= 1 &&
                        distance_col <= 1 &&
                        (distance_row != 0 ||
                         distance_col != 0);
                } else {
                    const bool straight =
                        (dr == 0 ||
                         dc == 0) &&
                        (dr != 0 ||
                         dc != 0);

                    const bool diagonal =
                        distance_row ==
                            distance_col &&
                        distance_row != 0;

                    const bool rook =
                        dynamic_cast<
                            const Rook*>(
                            piece) != nullptr;

                    const bool bishop =
                        dynamic_cast<
                            const Bishop*>(
                            piece) != nullptr;

                    const bool queen =
                        dynamic_cast<
                            const Queen*>(
                            piece) != nullptr;

                    if ((straight &&
                         (rook ||
                          queen)) ||
                        (diagonal &&
                         (bishop ||
                          queen))) {
                        const int step_row =
                            (dr > 0) -
                            (dr < 0);

                        const int step_col =
                            (dc > 0) -
                            (dc < 0);

                        attacks = true;

                        int scan_row =
                            row + step_row;
                        int scan_col =
                            col + step_col;

                        while (
                            scan_row !=
                                king_row ||
                            scan_col !=
                                king_col) {
                            if (board.is_there(
                                    scan_row,
                                    scan_col)) {
                                attacks =
                                    false;
                                break;
                            }

                            scan_row +=
                                step_row;
                            scan_col +=
                                step_col;
                        }
                    }
                }

                if (attacks) {
                    attackers.emplace_back(
                        row, col);
                }
            }
        }

        return attackers;
    }

    void refresh() {
        menu_panel_->setVisible(
            menu_open_);

        replay_panel_->setVisible(
            mode_ == Mode::Replay &&
            !menu_open_);

        menu_button_->setVisible(
            mode_ != Mode::None);

        menu_button_->setText(
            menu_open_
                ? "Resume"
                : "Menu");

        const bool flipped =
            board_flipped();

        const Game& displayed_game =
            mode_ == Mode::Replay
                ? replay_game_
                : game_;

        const Board& board =
            displayed_game.board();

        int checked_row = -1;
        int checked_col = -1;

        std::vector<
            std::pair<int, int>>
            attackers;

        if (displayed_game.in_check()) {
            const bool
                checked_white =
                    displayed_game
                        .white_to_move();

            for (int row = 0;
                 row < 8; ++row) {
                for (int col = 0;
                     col < 8; ++col) {
                    const Piece* piece =
                        board.get_piece(
                            row, col,
                            board);

                    if (piece &&
                        piece->is_white ==
                            checked_white &&
                        dynamic_cast<
                            const King*>(
                            piece)) {
                        checked_row = row;
                        checked_col = col;
                    }
                }
            }

            if (checked_row >= 0) {
                attackers =
                    checking_pieces(
                        board,
                        checked_white,
                        checked_row,
                        checked_col);
            }
        }

        for (int view_row = 0;
             view_row < 8;
             ++view_row) {
            const int row =
                flipped
                    ? 7 - view_row
                    : view_row;

            ranks_[view_row]->
                setText(
                    QString::number(
                        8 - row));

            for (int view_col = 0;
                 view_col < 8;
                 ++view_col) {
                const int col =
                    flipped
                        ? 7 - view_col
                        : view_col;

                auto* square =
                    squares_[
                        view_row][
                        view_col];

                draw_piece(
                    square,
                    board.get_piece(
                        row, col,
                        board));

                QString background =
                    (row + col) % 2
                        ? "#91a873"
                        : "#eee8d5";

                QString border =
                    "1px solid #758365";

                const bool is_attacker =
                    std::find(
                        attackers.begin(),
                        attackers.end(),
                        std::make_pair(
                            row, col)) !=
                    attackers.end();

                if (is_attacker) {
                    background =
                        (row + col) % 2
                            ? "#a2b4c3"
                            : "#d5e4ed";

                    border =
                        "2px solid #7297b2";
                }

                for (const auto& move :
                     legal_moves_) {
                    if (move.to_row ==
                            row &&
                        move.to_col ==
                            col) {
                        background =
                            "#a9d983";
                        border =
                            "2px solid "
                            "#487532";
                        break;
                    }
                }

                if (row ==
                        selected_row_ &&
                    col ==
                        selected_col_) {
                    background =
                        "#f3c969";
                    border =
                        "2px solid "
                        "#94701e";
                }

                // Red takes priority over
                // other square highlights.
                if (row ==
                        checked_row &&
                    col ==
                        checked_col) {
                    background =
                        (row + col) % 2
                            ? "#d98d88"
                            : "#f0b5af";

                    border =
                        "2px solid "
                        "#aa4843";
                }

                square->setStyleSheet(
                    QString(
                        "QPushButton {"
                        "background: %1;"
                        "color: #202522;"
                        "border: %2;"
                        "border-radius: 0px;"
                        "padding: 0px;"
                        "font-size: 19px;"
                        "font-weight: bold;"
                        "}")
                        .arg(
                            background,
                            border));

                square->setCursor(
                    human_can_move()
                        ? Qt::
                            PointingHandCursor
                        : Qt::
                            ArrowCursor);

                square->setToolTip(
                    QString(
                        QChar('a' +
                              col)) +
                    QString::number(
                        8 - row));
            }
        }

        for (int view_col = 0;
             view_col < 8;
             ++view_col) {
            const int col =
                flipped
                    ? 7 - view_col
                    : view_col;

            files_[view_col]->
                setText(
                    QString(
                        QChar('a' +
                              col)));
        }

        previous_->setEnabled(
            replay_loaded_ &&
            replay_position_ > 0);

        next_->setEnabled(
            replay_loaded_ &&
            replay_position_ <
                replay_moves_.size());

        replay_progress_->setText(
            QString("Move %1 / %2")
                .arg(
                    static_cast<
                        qulonglong>(
                        replay_position_))
                .arg(
                    static_cast<
                        qulonglong>(
                        replay_moves_
                            .size())));

        if (menu_open_) {
            status_->setText(
                mode_ == Mode::None
                    ? "Choose a "
                      "game mode."
                    : "Menu — choose "
                      "a mode or "
                      "press Resume.");
            return;
        }

        if (mode_ ==
            Mode::Replay) {
            if (!replay_loaded_) {
                status_->setText(
                    replay_source_->
                            currentIndex()
                            == 1
                        ? "Enter a random "
                          "game ID and "
                          "press Load."
                        : records_.empty()
                            ? "No saved "
                              "local games "
                              "yet."
                            : "Select a "
                              "valid saved "
                              "game.");
                return;
            }

            const QString source =
                replay_source_->
                        currentIndex() == 1
                    ? "Random"
                    : "Local";

            status_->setText(
                QString(
                    "%1 replay — game "
                    "%2 — %3 — %4\n"
                    "Move %5 of %6")
                    .arg(source)
                    .arg(
                        QString::number(
                            static_cast<
                                qulonglong>(
                                replay_record_
                                    .id)))
                    .arg(
                        QString::
                            fromStdString(
                                replay_record_
                                    .result))
                    .arg(
                        QString::
                            fromStdString(
                                replay_record_
                                    .reason))
                    .arg(
                        static_cast<
                            qulonglong>(
                            replay_position_))
                    .arg(
                        static_cast<
                            qulonglong>(
                            replay_moves_
                                .size())));
            return;
        }

        if (game_.finished()) {
            status_->setText(
                result_text());
            return;
        }

        QString text =
            game_.white_to_move()
                ? "White to move"
                : "Black to move";

        if (game_.in_check()) {
            text += " — check";
        }

        if (mode_ ==
            Mode::Random) {
            text +=
                game_.white_to_move()
                        == human_white_
                    ? " — your turn"
                    : " — random "
                      "opponent";
        }

        status_->setText(text);
    }
};

int main(
    int argc, char* argv[]) {
    QApplication app(argc, argv);

    try {
        ChessWindow window;
        window.show();
        return app.exec();
    } catch (
        const std::exception& error) {
        QMessageBox::critical(
            nullptr,
            "Could not start Chess",
            QString::fromUtf8(
                error.what()));
        return 1;
    }
}