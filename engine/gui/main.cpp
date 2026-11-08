#include "ai.hpp"
#include "game.hpp"
#include "history.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <future>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QLockFile>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QPushButton>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class BoardAnnotations final : public QWidget {
public:
    struct Square {
        int row;
        int col;

        bool operator==(const Square& other) const {
            return row == other.row && col == other.col;
        }
    };

    struct Arrow {
        Square from;
        Square to;

        bool operator==(const Arrow& other) const {
            return from == other.from && to == other.to;
        }
    };

    explicit BoardAnnotations(QWidget* parent) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setFocusPolicy(Qt::NoFocus);
    }

    void set_board_geometry(const QRect& board, bool flipped) {
        flipped_ = flipped;
        setGeometry(board);
        raise();
        update();
    }

    std::optional<Square> square_at(const QPoint& global_position) const {
        const QPoint local = mapFromGlobal(global_position);
        if (!rect().contains(local) || width() <= 0 || height() <= 0) {
            return std::nullopt;
        }

        const int row = local.y() * 8 / height();
        const int col = local.x() * 8 / width();
        return Square{
            flipped_ ? 7 - row : row,
            flipped_ ? 7 - col : col
        };
    }

    void begin_gesture(Square from) {
        gesture_start_ = from;
        preview_.reset();
        update();
    }

    void preview_gesture(std::optional<Square> to) {
        preview_.reset();
        if (gesture_start_ && to && !(*gesture_start_ == *to)) {
            preview_ = Arrow{*gesture_start_, *to};
        }
        update();
    }

    void finish_gesture(std::optional<Square> to) {
        if (gesture_start_ && to) {
            if (*gesture_start_ == *to) {
                toggle(marked_squares_, *to);
            } else {
                toggle(arrows_, Arrow{*gesture_start_, *to});
            }
        }
        gesture_start_.reset();
        preview_.reset();
        update();
    }

    void clear_manual() {
        gesture_start_.reset();
        preview_.reset();
        marked_squares_.clear();
        arrows_.clear();
        update();
    }

    void reset() {
        ai_move_.reset();
        clear_manual();
    }

    void clear_ai_move() {
        ai_move_.reset();
        update();
    }

    void set_ai_move(const GameMove& move) {
        ai_move_ = Arrow{
            {move.from_row, move.from_col},
            {move.to_row, move.to_col}
        };
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);

        for (const auto& square : marked_squares_) {
            painter.fillRect(square_rect(square), QColor(122, 59, 173, 75));
        }
        if (ai_move_) {
            draw_arrow(
                painter, *ai_move_, QColor(255, 214, 112, 155), 0.13);
        }
        for (const auto& arrow : arrows_) {
            draw_arrow(painter, arrow, QColor(0, 0, 0, 210), 0.16);
        }
        if (preview_) {
            draw_arrow(painter, *preview_, QColor(0, 0, 0, 130), 0.16);
        }
    }

private:
    bool flipped_ = false;
    std::vector<Square> marked_squares_;
    std::vector<Arrow> arrows_;
    std::optional<Square> gesture_start_;
    std::optional<Arrow> preview_;
    std::optional<Arrow> ai_move_;

    template <typename Item>
    static void toggle(std::vector<Item>& items, const Item& item) {
        const auto existing = std::find(items.begin(), items.end(), item);
        if (existing == items.end()) {
            items.push_back(item);
        } else {
            items.erase(existing);
        }
    }

    QRectF square_rect(Square square) const {
        const int row = flipped_ ? 7 - square.row : square.row;
        const int col = flipped_ ? 7 - square.col : square.col;
        const qreal cell_width = width() / 8.0;
        const qreal cell_height = height() / 8.0;

        return QRectF(
            col * cell_width, row * cell_height,
            cell_width, cell_height);
    }

    void draw_arrow(
        QPainter& painter, const Arrow& arrow,
        const QColor& color, qreal width_fraction) const {

        const QPointF start = square_rect(arrow.from).center();
        const QPointF end = square_rect(arrow.to).center();
        const int rows = std::abs(arrow.to.row - arrow.from.row);
        const int cols = std::abs(arrow.to.col - arrow.from.col);
        const bool knight =
            (rows == 2 && cols == 1) || (rows == 1 && cols == 2);

        QPointF last_segment_start = start;
        QPainterPath shaft(start);

        if (knight) {
            const QPointF elbow = rows == 2
                ? QPointF(start.x(), end.y())
                : QPointF(end.x(), start.y());
            shaft.lineTo(elbow);
            last_segment_start = elbow;
        }

        const QPointF delta = end - last_segment_start;
        const qreal length = std::hypot(delta.x(), delta.y());
        if (length <= 0) return;

        const QPointF direction = delta / length;
        const QPointF perpendicular(-direction.y(), direction.x());
        const qreal cell = std::min(width(), height()) / 8.0;
        const QPointF head_base = end - direction * (cell * 0.30);
        const QPointF wing = perpendicular * (cell * 0.23);
        shaft.lineTo(head_base);

        QPen pen(color);
        pen.setWidthF(cell * width_fraction);
        pen.setCapStyle(Qt::FlatCap);
        pen.setJoinStyle(Qt::MiterJoin);

        painter.save();
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(shaft);

        QPolygonF head;
        head << end << head_base + wing << head_base - wing;
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawPolygon(head);
        painter.restore();
    }
};

class ChessWindow : public QWidget {
public:
    ChessWindow() {
        setWindowTitle("ZergChess");

        assets_ = QDir(QString::fromUtf8(CHESS_ASSET_PATH));
        save_path_ = QString::fromUtf8(CHESS_HISTORY_PATH);

        if (!QDir().mkpath(QFileInfo(save_path_).absolutePath())) {
            throw std::runtime_error(
                "Could not create the history directory.");
        }

        history_lock_ =
            std::make_unique<QLockFile>(save_path_ + ".lock");

        if (!history_lock_->tryLock(0)) {
            throw std::runtime_error(
                "Could not lock history. Another ZergChess window may be open.");
        }

        auto* layout = new QVBoxLayout(this);
        auto* header = new QHBoxLayout;

        status_ = new QLabel(this);
        status_->setWordWrap(true);
        header->addWidget(status_, 1);

        move_counter_ = new QLabel(this);
        move_counter_->setAlignment(Qt::AlignRight | Qt::AlignTop);
        header->addWidget(move_counter_, 0, Qt::AlignTop);
        layout->addLayout(header);

        auto* board_layout = new QGridLayout;
        board_layout->setSpacing(0);
        layout->addLayout(board_layout);

        for (int row = 0; row < 8; ++row) {
            ranks_[row] = new QLabel(this);
            board_layout->addWidget(ranks_[row], row, 0, Qt::AlignCenter);

            for (int col = 0; col < 8; ++col) {
                auto* square = new QPushButton(this);
                square->setFixedSize(64, 64);
                square->setIconSize(QSize(52, 52));
                square->setFocusPolicy(Qt::NoFocus);
                squares_[row][col] = square;
                square->installEventFilter(this);
                board_layout->addWidget(square, row, col + 1);

                connect(
                    square, &QPushButton::clicked, this,
                    [this, row, col] {
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

        live_review_panel_ = new QWidget(this);
        auto* live_review_layout = new QHBoxLayout(live_review_panel_);
        live_review_layout->setContentsMargins(0, 0, 0, 0);

        live_previous_ =
            new QPushButton("← Previous", live_review_panel_);
        live_next_ =
            new QPushButton("Next →", live_review_panel_);
        return_to_live_ =
            new QPushButton("Return to live", live_review_panel_);

        live_review_layout->addStretch();
        live_review_layout->addWidget(live_previous_);
        live_review_layout->addWidget(live_next_);
        live_review_layout->addWidget(return_to_live_);
        live_review_layout->addStretch();
        layout->addWidget(live_review_panel_);

        connect(live_previous_, &QPushButton::clicked, this, [this] {
            const auto position =
                live_review_ply_.value_or(live_moves_.size());
            if (position > 0) show_live_review(position - 1);
        });

        connect(live_next_, &QPushButton::clicked, this, [this] {
            if (live_review_ply_) {
                show_live_review(*live_review_ply_ + 1);
            }
        });

        connect(return_to_live_, &QPushButton::clicked, this, [this] {
            show_live_review(live_moves_.size());
        });

        menu_panel_ = new QWidget(this);
        auto* menu_layout = new QHBoxLayout(menu_panel_);
        menu_layout->setContentsMargins(0, 0, 0, 0);

        auto add_menu_button = [&](const QString& text, auto action) {
            auto* button = new QPushButton(text, menu_panel_);
            menu_layout->addWidget(button);
            connect(button, &QPushButton::clicked, this, action);
        };

        add_menu_button("Local", [this] {
            start_game(Mode::Local, true);
        });
        add_menu_button("Random", [this] {
            choose_side(Mode::Random);
        });
        add_menu_button("AI", [this] {
            choose_side(Mode::AI);
        });
        add_menu_button("Replay", [this] {
            open_history();
        });
        add_menu_button("Options", [this] {
            show_options();
        });
        layout->addWidget(menu_panel_);

        replay_panel_ = new QWidget(this);
        auto* replay_layout = new QVBoxLayout(replay_panel_);
        replay_layout->setContentsMargins(0, 0, 0, 0);

        auto* selection = new QHBoxLayout;
        games_box_ = new QComboBox(this);
        games_box_->setMinimumWidth(240);
        id_box_ = new QLineEdit(this);
        id_box_->setPlaceholderText("Game ID");
        id_box_->setMaximumWidth(120);

        auto* load = new QPushButton("Load", this);
        selection->addWidget(games_box_, 1);
        selection->addWidget(id_box_);
        selection->addWidget(load);
        replay_layout->addLayout(selection);

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

        connect(
            games_box_, &QComboBox::currentIndexChanged,
            this, [this](int index) {
                if (index >= 0 &&
                    index < static_cast<int>(records_.size())) {
                    load_replay(records_[index]);
                }
            });

        connect(load, &QPushButton::clicked, this, [this] {
            load_typed_id();
        });
        connect(id_box_, &QLineEdit::returnPressed, this, [this] {
            load_typed_id();
        });
        connect(previous_, &QPushButton::clicked, this, [this] {
            if (replay_position_ > 0) {
                show_replay_position(replay_position_ - 1);
            }
        });
        connect(next_, &QPushButton::clicked, this, [this] {
            if (replay_position_ < replay_moves_.size()) {
                show_replay_position(replay_position_ + 1);
            }
        });

        menu_button_ = new QPushButton("Menu", this);
        layout->addWidget(menu_button_);
        connect(menu_button_, &QPushButton::clicked, this, [this] {
            menu_open_ = !menu_open_;
            invalidate_search();
            clear_selection();
            refresh();
        });

        annotations_ = new BoardAnnotations(this);
        annotations_->show();

        timer_ = new QTimer(this);
        connect(timer_, &QTimer::timeout, this, [this] {
            tick();
        });
        timer_->start(25);
        refresh();
    }

    ~ChessWindow() override {
        timer_->stop();
        invalidate_search();
        if (worker_.valid()) worker_.wait();
        if (ponder_worker_.valid()) ponder_worker_.wait();
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (!annotations_ || live_review_ply_) {
            return QWidget::eventFilter(watched, event);
        }

        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick: {
            auto* mouse = static_cast<QMouseEvent*>(event);
            if (mouse->button() == Qt::LeftButton) {
                annotations_->clear_manual();
                break;
            }
            if (mouse->button() == Qt::RightButton) {
                const auto square = annotations_->square_at(
                    mouse->globalPosition().toPoint());
                if (square) annotations_->begin_gesture(*square);
                return true;
            }
            break;
        }
        case QEvent::MouseMove: {
            auto* mouse = static_cast<QMouseEvent*>(event);
            if (mouse->buttons().testFlag(Qt::RightButton)) {
                annotations_->preview_gesture(
                    annotations_->square_at(
                        mouse->globalPosition().toPoint()));
                return true;
            }
            break;
        }
        case QEvent::MouseButtonRelease: {
            auto* mouse = static_cast<QMouseEvent*>(event);
            if (mouse->button() == Qt::RightButton) {
                annotations_->finish_gesture(
                    annotations_->square_at(
                        mouse->globalPosition().toPoint()));
                return true;
            }
            break;
        }
        case QEvent::ContextMenu:
            return true;
        default:
            break;
        }

        return QWidget::eventFilter(watched, event);
    }

    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);
        sync_annotations();
    }

private:
    enum class Mode { None, Local, Random, AI, Replay };

    Game game_;
    Game replay_game_;
    std::vector<GameMove> live_moves_;
    std::optional<Game> live_review_game_;
    std::optional<std::size_t> live_review_ply_;

    QLabel* move_counter_ = nullptr;
    QWidget* live_review_panel_ = nullptr;
    QPushButton* live_previous_ = nullptr;
    QPushButton* live_next_ = nullptr;
    QPushButton* return_to_live_ = nullptr;

    Mode mode_ = Mode::None;
    bool menu_open_ = true;
    bool human_white_ = true;
    bool started_ = false;

    QDir assets_;
    QString save_path_;
    std::unique_ptr<QLockFile> history_lock_;

    QLabel* status_ = nullptr;
    QWidget* menu_panel_ = nullptr;
    QWidget* replay_panel_ = nullptr;
    QPushButton* menu_button_ = nullptr;
    QTimer* timer_ = nullptr;
    BoardAnnotations* annotations_ = nullptr;

    std::array<QLabel*, 8> ranks_{};
    std::array<QLabel*, 8> files_{};
    std::array<std::array<QPushButton*, 8>, 8> squares_{};

    int selected_row_ = -1;
    int selected_col_ = -1;
    std::vector<GameMove> legal_moves_;

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

    zerg::SearchLimits limits_;
    const int available_workers_ = zerg::available_workers();

    std::future<zerg::SearchResult> worker_;
    std::shared_ptr<std::atomic_bool> cancellation_;
    std::uint64_t generation_ = 0;
    std::uint64_t worker_generation_ = 0;

    bool pondering_enabled_ = true;
    std::future<std::vector<zerg::PonderReply>> ponder_worker_;
    std::shared_ptr<std::atomic_bool> ponder_cancellation_;
    std::optional<std::uint64_t> ponder_generation_;
    std::optional<GameMove> pending_human_move_;
    std::optional<zerg::SearchResult> ponder_seed_;

    QString last_search_;
    std::mt19937 random_{std::random_device{}()};

    static bool same_move(const GameMove& a, const GameMove& b) {
        return a.from_row == b.from_row &&
               a.from_col == b.from_col &&
               a.to_row == b.to_row &&
               a.to_col == b.to_col &&
               a.promotion == b.promotion;
    }

    void sync_annotations() {
        if (!annotations_) return;
        if (layout()) layout()->activate();

        const QRect board = squares_[0][0]->geometry().united(
            squares_[7][7]->geometry());
        annotations_->set_board_geometry(board, board_flipped());
    }

    bool live_mode() const {
        return mode_ == Mode::Local ||
               mode_ == Mode::Random ||
               mode_ == Mode::AI;
    }

    bool board_flipped() const {
        if (mode_ == Mode::Local) return !game_.white_to_move();
        if (mode_ == Mode::Random || mode_ == Mode::AI) {
            return !human_white_;
        }
        return false;
    }

    bool human_can_move() const {
        return !menu_open_ && !live_review_ply_ &&
               live_mode() && !game_.finished() &&
               (mode_ == Mode::Local ||
                game_.white_to_move() == human_white_);
    }

    bool bot_turn() const {
        return !menu_open_ && !live_review_ply_ &&
               !game_.finished() &&
               (mode_ == Mode::AI || mode_ == Mode::Random) &&
               game_.white_to_move() != human_white_;
    }

    void invalidate_search() {
        ++generation_;
        if (cancellation_) {
            cancellation_->store(true, std::memory_order_relaxed);
        }
        if (ponder_cancellation_) {
            ponder_cancellation_->store(true, std::memory_order_relaxed);
        }

        pending_human_move_.reset();
        ponder_seed_.reset();
        ponder_generation_.reset();
    }

    void clear_selection() {
        selected_row_ = selected_col_ = -1;
        legal_moves_.clear();
    }

    bool confirm_discard() {
        if (!live_mode() || !started_ || game_.finished()) return true;

        return QMessageBox::question(
            this, "Leave current game",
            "Discard the unfinished game?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No) == QMessageBox::Yes;
    }

    void choose_side(Mode mode) {
        bool confirmed = false;
        const QString choice = QInputDialog::getItem(
            this,
            mode == Mode::AI ? "AI opponent" : "Random opponent",
            "Play as:", QStringList{"White", "Black"},
            0, false, &confirmed);

        if (confirmed) start_game(mode, choice == "White");
    }

    void start_game(Mode mode, bool human_white) {
        if (!confirm_discard()) return;

        annotations_->reset();
        invalidate_search();
        game_ = Game{};
        live_moves_.clear();
        live_review_game_.reset();
        live_review_ply_.reset();
        mode_ = mode;
        human_white_ = human_white;
        menu_open_ = false;
        started_ = false;
        last_search_.clear();
        clear_selection();
        refresh();
    }

    void show_live_review(std::size_t position) {
        if (!live_mode() || menu_open_ ||
            position > live_moves_.size()) {
            return;
        }

        if (position == live_moves_.size()) {
            live_review_game_.reset();
            live_review_ply_.reset();
        } else {
            Game reviewed;
            for (std::size_t ply = 0; ply < position; ++ply) {
                if (!reviewed.play(live_moves_[ply])) {
                    QMessageBox::warning(
                        this, "Review error",
                        "Could not reconstruct that position.");
                    return;
                }
            }

            live_review_game_ = std::move(reviewed);
            live_review_ply_ = position;
            invalidate_search();
        }

        clear_selection();
        annotations_->clear_manual();
        refresh();
    }

    void refresh_live_review_controls() {
        const bool reviewing = live_review_ply_.has_value();
        const bool show_controls = live_mode() && !menu_open_;
        const std::size_t latest = live_moves_.size();
        const std::size_t shown = live_review_ply_.value_or(latest);

        live_review_panel_->setVisible(show_controls);
        live_previous_->setEnabled(show_controls && shown > 0);
        live_next_->setEnabled(show_controls && reviewing);
        return_to_live_->setEnabled(show_controls && reviewing);

        move_counter_->setVisible(live_mode());
        if (live_mode()) {
            QString counter = QString("Move %1 / %2")
                .arg(static_cast<qulonglong>(shown))
                .arg(static_cast<qulonglong>(latest));

            if (reviewing) {
                counter += QString(" · %1 back")
                    .arg(static_cast<qulonglong>(latest - shown));
            }
            move_counter_->setText(counter);
        }

        annotations_->setVisible(!reviewing);
    }

    void show_options() {
        QDialog dialog(this);
        dialog.setWindowTitle("AI options");
        auto* form = new QFormLayout(&dialog);

        auto* depth = new QSpinBox(&dialog);
        depth->setRange(1, 13);
        depth->setValue(limits_.max_depth);

        auto* time = new QSpinBox(&dialog);
        time->setRange(1, 3600);
        time->setSingleStep(1);
        time->setSuffix(" s");
        time->setValue(std::max(1, limits_.time_ms / 1000));

        auto* nodes = new QSpinBox(&dialog);
        nodes->setRange(100, 1000000000);
        nodes->setSingleStep(1000);
        nodes->setValue(static_cast<int>(limits_.max_nodes));

        auto* workers = new QSpinBox(&dialog);
        workers->setRange(1, available_workers_);
        workers->setValue(limits_.workers);

        auto* all_workers = new QPushButton(
            QString("Use all %1").arg(available_workers_), &dialog);

        connect(
            all_workers, &QPushButton::clicked, &dialog,
            [workers, this] {
                workers->setValue(available_workers_);
            });

        auto* worker_row = new QWidget(&dialog);
        auto* worker_layout = new QHBoxLayout(worker_row);
        worker_layout->setContentsMargins(0, 0, 0, 0);
        worker_layout->addWidget(workers);
        worker_layout->addWidget(all_workers);

        auto* pondering = new QCheckBox(
            "Think during my turn", &dialog);
        pondering->setChecked(pondering_enabled_);

        form->addRow("CPU workers", worker_row);
        form->addRow("Normal depth limit (halfmoves)", depth);
        form->addRow("Thinking time per AI move", time);
        form->addRow("Maximum nodes per AI move", nodes);
        form->addRow(pondering);

        auto* explanation = new QLabel(
            QString(
                "%1 logical CPUs available. Default: 1 worker.\n"
                "Time and node limits are shared across workers.\n"
                "Pondering continues during your turn and can search\n"
                "beyond the normal depth limit. After your move, the AI\n"
                "keeps the matching reply and tries to search deeper\n"
                "within its normal time and node budgets.")
                .arg(available_workers_),
            &dialog);
        explanation->setWordWrap(true);
        form->addRow(explanation);

        auto* buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
            &dialog);
        form->addRow(buttons);

        connect(
            buttons, &QDialogButtonBox::accepted,
            &dialog, &QDialog::accept);
        connect(
            buttons, &QDialogButtonBox::rejected,
            &dialog, &QDialog::reject);

        if (dialog.exec() == QDialog::Accepted) {
            limits_.max_depth = depth->value();
            limits_.time_ms = time->value() * 1000;
            limits_.max_nodes =
                static_cast<std::uint64_t>(nodes->value());
            limits_.workers = workers->value();
            pondering_enabled_ = pondering->isChecked();
            invalidate_search();
            refresh();
        }
    }

    void tick() {
        // Drain pondering before starting an AI-turn search.
        // Polling keeps the GUI responsive while workers stop.
        if (ponder_worker_.valid()) {
            if (ponder_worker_.wait_for(std::chrono::milliseconds(0)) !=
                std::future_status::ready) {
                return;
            }

            const bool same_human_turn =
                ponder_generation_ &&
                *ponder_generation_ == generation_;

            const bool matching_move =
                ponder_generation_ &&
                pending_human_move_ &&
                *ponder_generation_ + 1 == generation_ &&
                mode_ == Mode::AI && bot_turn();

            try {
                // All unused replies are destroyed at the end of this block,
                // before the AI-turn search starts.
                auto replies = ponder_worker_.get();
                ponder_cancellation_.reset();

                if (matching_move) {
                    for (const auto& entry : replies) {
                        if (same_move(
                                entry.human_move, *pending_human_move_) &&
                            entry.reply.move &&
                            entry.reply.completed_depth > 0) {
                            ponder_seed_ = entry.reply;
                            break;
                        }
                    }
                }
            } catch (const std::exception& error) {
                ponder_cancellation_.reset();

                if (same_human_turn || matching_move) {
                    invalidate_search();
                    menu_open_ = true;
                    refresh();
                    QMessageBox::warning(
                        this, "Pondering error",
                        QString::fromUtf8(error.what()));
                    return;
                }
            }

            pending_human_move_.reset();

            // Retain the marker if this session naturally reached its cap.
            // This avoids repeatedly restarting completed pondering.
            if (!same_human_turn) ponder_generation_.reset();
            refresh();
        }

        if (worker_.valid()) {
            if (worker_.wait_for(std::chrono::milliseconds(0)) !=
                std::future_status::ready) {
                return;
            }

            const bool current = worker_generation_ == generation_;

            try {
                const auto result = worker_.get();
                cancellation_.reset();

                if (current && mode_ == Mode::AI && bot_turn()) {
                    last_search_ =
                        QString("AI: depth %1, %2 new nodes, %3 ms%4")
                            .arg(result.completed_depth)
                            .arg(static_cast<qulonglong>(result.nodes))
                            .arg(static_cast<qlonglong>(result.elapsed_ms))
                            .arg(result.completed_depth == 0
                                ? " — legal fallback" : "");

                    if (!result.move) {
                        throw std::runtime_error(
                            "AI returned no move for an unfinished game.");
                    }
                    accept_move(*result.move);
                }
            } catch (const std::exception& error) {
                cancellation_.reset();

                if (current) {
                    invalidate_search();
                    menu_open_ = true;
                    refresh();
                    QMessageBox::warning(
                        this, "AI error",
                        QString::fromUtf8(error.what()));
                }
            }
            return;
        }

        const bool can_ponder =
            mode_ == Mode::AI &&
            pondering_enabled_ &&
            human_can_move();

        if (can_ponder) {
            if (ponder_generation_ &&
                *ponder_generation_ == generation_) {
                return;
            }

            try {
                Game snapshot = game_;
                auto flag = std::make_shared<std::atomic_bool>(false);
                const auto limits = limits_;

                ponder_cancellation_ = flag;
                ponder_generation_ = generation_;

                ponder_worker_ = std::async(
                    std::launch::async,
                    [snapshot = std::move(snapshot), flag, limits] {
                        return zerg::ponder(
                            snapshot, limits, flag.get());
                    });
                refresh();
            } catch (const std::exception& error) {
                invalidate_search();
                menu_open_ = true;
                refresh();
                QMessageBox::warning(
                    this, "Cannot start pondering",
                    QString::fromUtf8(error.what()));
            }
            return;
        }

        if (!bot_turn()) return;

        if (mode_ == Mode::Random) {
            std::vector<GameMove> moves;
            for (int row = 0; row < 8; ++row) {
                for (int col = 0; col < 8; ++col) {
                    auto from = game_.legal_moves_from(row, col);
                    moves.insert(moves.end(), from.begin(), from.end());
                }
            }

            if (!moves.empty()) {
                std::uniform_int_distribution<std::size_t> pick(
                    0, moves.size() - 1);
                accept_move(moves[pick(random_)]);
            }
            return;
        }

        try {
            Game snapshot = game_;
            auto flag = std::make_shared<std::atomic_bool>(false);
            const auto limits = limits_;
            const auto seed = ponder_seed_;
            ponder_seed_.reset();

            worker_generation_ = generation_;
            cancellation_ = flag;

            worker_ = std::async(
                std::launch::async,
                [snapshot = std::move(snapshot), flag, limits, seed] {
                    return zerg::choose_move(
                        snapshot, limits, flag.get(), zerg::evaluate,
                        seed ? &*seed : nullptr);
                });
            refresh();
        } catch (const std::exception& error) {
            invalidate_search();
            cancellation_.reset();
            menu_open_ = true;
            refresh();
            QMessageBox::warning(
                this, "Cannot start AI",
                QString::fromUtf8(error.what()));
        }
    }

    QString result_text() const {
        if (game_.reason() == "checkmate") {
            return game_.result() == "1-0"
                ? "White wins by checkmate!"
                : "Black wins by checkmate!";
        }
        return "Draw: " + QString::fromStdString(game_.reason());
    }

    void accept_move(const GameMove& move) {
        if (!game_.play(move)) {
            clear_selection();
            refresh();
            return;
        }

        // play() has switched turns.
        const bool human_just_moved =
            mode_ == Mode::AI &&
            game_.white_to_move() != human_white_;

        if (human_just_moved) {
            ponder_seed_.reset();

            if (ponder_worker_.valid() &&
                ponder_generation_ &&
                *ponder_generation_ == generation_) {
                pending_human_move_ = move;
                if (ponder_cancellation_) {
                    ponder_cancellation_->store(
                        true, std::memory_order_relaxed);
                }
            } else {
                pending_human_move_.reset();
            }
        }

        live_moves_.push_back(move);

        if (mode_ == Mode::AI) {
            if (game_.white_to_move() == human_white_) {
                annotations_->set_ai_move(move);
            } else {
                annotations_->clear_ai_move();
            }
        }

        ++generation_;
        started_ = true;
        clear_selection();

        if (game_.finished()) {
            invalidate_search();
        }

        refresh();

        if (game_.finished()) {
            const bool saved =
                game_.save_finished_game(save_path_.toStdString());

            QMessageBox dialog(this);
            dialog.setWindowTitle("Game over");
            dialog.setTextFormat(Qt::PlainText);
            dialog.setIcon(
                saved ? QMessageBox::Information : QMessageBox::Warning);
            dialog.setText(result_text());
            dialog.setInformativeText(
                (saved ? "Game saved to:\n" : "Could not save game to:\n") +
                save_path_);
            dialog.exec();
        }
    }

    void click_square(int row, int col) {
        if (!human_can_move()) return;

        const auto destination = std::find_if(
            legal_moves_.begin(), legal_moves_.end(),
            [=](const GameMove& move) {
                return move.to_row == row && move.to_col == col;
            });

        if (destination != legal_moves_.end()) {
            GameMove move = *destination;

            if (move.promotion != '\0') {
                bool confirmed = false;
                const QStringList names{
                    "Queen", "Rook", "Bishop", "Knight"
                };
                const QString choice = QInputDialog::getItem(
                    this, "Promotion", "Promote to:",
                    names, 0, false, &confirmed);
                const auto index = names.indexOf(choice);
                if (!confirmed || index < 0) return;
                move.promotion = "qrbn"[index];
            }

            accept_move(move);
            return;
        }

        const bool deselect =
            row == selected_row_ && col == selected_col_;
        clear_selection();

        const Board& board = game_.board();
        const Piece* piece = board.get_piece(row, col, board);

        if (!deselect && piece &&
            piece->is_white == game_.white_to_move()) {
            selected_row_ = row;
            selected_col_ = col;
            legal_moves_ = game_.legal_moves_from(row, col);
        }
        refresh();
    }

    static GameMove parse_move(const std::string& text) {
        if ((text.size() != 4 && text.size() != 5) ||
            text[0] < 'a' || text[0] > 'h' ||
            text[1] < '1' || text[1] > '8' ||
            text[2] < 'a' || text[2] > 'h' ||
            text[3] < '1' || text[3] > '8' ||
            (text.size() == 5 &&
             std::string("qrbn").find(text[4]) == std::string::npos)) {
            throw std::runtime_error("Invalid move: " + text);
        }

        return {
            '8' - text[1], text[0] - 'a',
            '8' - text[3], text[2] - 'a',
            text.size() == 5 ? text[4] : '\0'
        };
    }

    static std::string placement(const Board& board) {
        std::string result;

        for (int row = 0; row < 8; ++row) {
            if (row) result += '/';
            int empty = 0;

            for (int col = 0; col < 8; ++col) {
                const Piece* piece = board.get_piece(row, col, board);
                if (!piece) {
                    ++empty;
                    continue;
                }
                if (empty) {
                    result += static_cast<char>('0' + empty);
                    empty = 0;
                }

                char letter =
                    piece->name == "Knight" ? 'N' : piece->name.front();
                if (!piece->is_white) letter += 'a' - 'A';
                result += letter;
            }

            if (empty) result += static_cast<char>('0' + empty);
        }

        return result;
    }

    void open_history() {
        if (!confirm_discard()) return;

        annotations_->reset();
        invalidate_search();
        mode_ = Mode::Replay;
        live_moves_.clear();
        live_review_game_.reset();
        live_review_ply_.reset();
        menu_open_ = false;
        started_ = false;
        replay_loaded_ = false;
        replay_moves_.clear();
        replay_position_ = 0;
        replay_game_ = Game{};
        records_.clear();
        clear_selection();

        {
            const QSignalBlocker blocker(games_box_);
            games_box_->clear();
        }

        try {
            records_ = history::load_and_migrate(
                save_path_.toStdString()).records;
            std::reverse(records_.begin(), records_.end());

            {
                const QSignalBlocker blocker(games_box_);
                for (const auto& record : records_) {
                    games_box_->addItem(
                        QString("Game %1 — %2")
                            .arg(static_cast<qulonglong>(record.id))
                            .arg(QString::fromStdString(record.result)));
                }
            }

            if (!records_.empty()) load_replay(records_.front());
        } catch (const std::exception& error) {
            QMessageBox::warning(
                this, "History error", QString::fromUtf8(error.what()));
        }

        refresh();
    }

    void load_typed_id() {
        try {
            const auto id =
                history::parse_id(id_box_->text().trimmed().toStdString());

            const auto found = std::find_if(
                records_.begin(), records_.end(),
                [id](const auto& record) {
                    return record.id == id;
                });

            if (found == records_.end()) {
                throw std::runtime_error("No saved game has that ID.");
            }

            {
                const QSignalBlocker blocker(games_box_);
                games_box_->setCurrentIndex(
                    static_cast<int>(found - records_.begin()));
            }
            load_replay(*found);
        } catch (const std::exception& error) {
            QMessageBox::warning(
                this, "Cannot load game", QString::fromUtf8(error.what()));
        }
    }

    void load_replay(const history::Record& record) {
        annotations_->reset();

        try {
            Game validation;
            std::vector<GameMove> moves;
            std::istringstream input(record.moves);
            std::string text;

            while (input >> text) {
                const auto move = parse_move(text);
                if (!validation.play(move)) {
                    throw std::runtime_error(
                        "Illegal recorded move: " + text);
                }
                moves.push_back(move);
            }

            const bool completed =
                validation.finished() &&
                validation.result() == record.result &&
                validation.reason() == record.reason;

            const bool unfinished =
                !validation.finished() &&
                record.result == "unfinished" &&
                record.reason == "move_limit";

            if (placement(validation.board()) != record.final_placement ||
                (!completed && !unfinished)) {
                throw std::runtime_error(
                    "Saved position or result does not match the replay.");
            }

            replay_record_ = record;
            replay_moves_ = std::move(moves);
            replay_game_ = Game{};
            replay_position_ = 0;
            replay_loaded_ = true;
            id_box_->setText(QString::number(
                static_cast<qulonglong>(record.id)));
        } catch (const std::exception& error) {
            replay_loaded_ = false;
            replay_moves_.clear();
            replay_position_ = 0;
            replay_game_ = Game{};
            QMessageBox::warning(
                this, "Replay error", QString::fromUtf8(error.what()));
        }

        refresh();
    }

    void show_replay_position(std::size_t position) {
        if (!replay_loaded_ || position > replay_moves_.size()) return;

        if (position < replay_position_) {
            replay_game_ = Game{};
            replay_position_ = 0;
        }

        while (replay_position_ < position) {
            if (!replay_game_.play(replay_moves_[replay_position_])) {
                replay_loaded_ = false;
                QMessageBox::warning(
                    this, "Replay error",
                    "Could not reconstruct the game.");
                break;
            }
            ++replay_position_;
        }

        refresh();
    }

    static bool attacks(
        const Board& board, const Piece& piece,
        int row, int col, int king_row, int king_col) {

        const int dr = king_row - row;
        const int dc = king_col - col;
        const int ar = std::abs(dr);
        const int ac = std::abs(dc);

        if (piece.name == "Pawn") {
            return dr == (piece.is_white ? -1 : 1) && ac == 1;
        }
        if (piece.name == "Knight") {
            return (ar == 2 && ac == 1) || (ar == 1 && ac == 2);
        }
        if (piece.name == "King") {
            return std::max(ar, ac) == 1;
        }

        const bool straight = (dr == 0 || dc == 0) && (dr || dc);
        const bool diagonal = ar == ac && ar != 0;
        const bool slider =
            (straight &&
             (piece.name == "Rook" || piece.name == "Queen")) ||
            (diagonal &&
             (piece.name == "Bishop" || piece.name == "Queen"));

        if (!slider) return false;

        const int sr = (dr > 0) - (dr < 0);
        const int sc = (dc > 0) - (dc < 0);

        for (int r = row + sr, c = col + sc;
             r != king_row || c != king_col;
             r += sr, c += sc) {
            if (board.is_there(r, c)) return false;
        }

        return true;
    }

    void refresh() {
        menu_panel_->setVisible(menu_open_);
        replay_panel_->setVisible(mode_ == Mode::Replay && !menu_open_);
        menu_button_->setVisible(mode_ != Mode::None);
        menu_button_->setText(menu_open_ ? "Resume" : "Menu");

        const Game& displayed = mode_ == Mode::Replay
            ? replay_game_
            : (live_review_game_ ? *live_review_game_ : game_);

        const Board& board = displayed.board();
        const bool flipped = board_flipped();

        int king_row = -1;
        int king_col = -1;

        if (displayed.in_check()) {
            for (int row = 0; row < 8; ++row) {
                for (int col = 0; col < 8; ++col) {
                    const Piece* piece = board.get_piece(row, col, board);
                    if (piece && piece->name == "King" &&
                        piece->is_white == displayed.white_to_move()) {
                        king_row = row;
                        king_col = col;
                    }
                }
            }
        }

        for (int view_row = 0; view_row < 8; ++view_row) {
            const int row = flipped ? 7 - view_row : view_row;
            ranks_[view_row]->setText(QString::number(8 - row));

            for (int view_col = 0; view_col < 8; ++view_col) {
                const int col = flipped ? 7 - view_col : view_col;
                auto* square = squares_[view_row][view_col];
                const Piece* piece = board.get_piece(row, col, board);

                square->setIcon(QIcon());
                square->setText(QString());

                if (piece) {
                    const QString filename =
                        QString::fromStdString(piece->name).toLower() +
                        (piece->is_white ? "-w.svg" : "-b.svg");

                    const QIcon icon(assets_.filePath(filename));
                    if (icon.pixmap(square->iconSize()).isNull()) {
                        square->setText(
                            QString::fromStdString(piece->symbol));
                    } else {
                        square->setIcon(icon);
                    }
                }

                QString background =
                    (row + col) % 2 ? "#91a873" : "#eee8d5";
                QString border = "1px solid #758365";

                if (king_row >= 0 && piece &&
                    piece->is_white != displayed.white_to_move() &&
                    attacks(board, *piece, row, col, king_row, king_col)) {
                    background = "#c4dbe9";
                    border = "2px solid #7297b2";
                }

                for (const auto& move : legal_moves_) {
                    if (move.to_row == row && move.to_col == col) {
                        background = "#a9d983";
                        border = "2px solid #487532";
                        break;
                    }
                }

                if (row == selected_row_ && col == selected_col_) {
                    background = "#f3c969";
                    border = "2px solid #94701e";
                }

                if (row == king_row && col == king_col) {
                    background = "#f0b5af";
                    border = "2px solid #aa4843";
                }

                square->setStyleSheet(
                    QString(
                        "QPushButton { background: %1; color: #202522;"
                        "border: %2; border-radius: 0px; padding: 0px;"
                        "font-size: 19px; font-weight: bold; }")
                        .arg(background, border));

                square->setCursor(
                    human_can_move()
                        ? Qt::PointingHandCursor
                        : Qt::ArrowCursor);

                square->setToolTip(
                    QString(QChar('a' + col)) + QString::number(8 - row));
            }
        }

        for (int view_col = 0; view_col < 8; ++view_col) {
            const int col = flipped ? 7 - view_col : view_col;
            files_[view_col]->setText(QString(QChar('a' + col)));
        }

        previous_->setEnabled(replay_loaded_ && replay_position_ > 0);
        next_->setEnabled(
            replay_loaded_ && replay_position_ < replay_moves_.size());

        replay_progress_->setText(
            QString("Ply %1 / %2")
                .arg(static_cast<qulonglong>(replay_position_))
                .arg(static_cast<qulonglong>(replay_moves_.size())));

        if (menu_open_) {
            status_->setText(
                mode_ == Mode::None
                    ? "Choose a game mode."
                    : "Choose a mode or press Resume.");
        } else if (mode_ == Mode::Replay) {
            status_->setText(
                replay_loaded_
                    ? QString("Replay — game %1 — %2 — %3")
                        .arg(static_cast<qulonglong>(replay_record_.id))
                        .arg(QString::fromStdString(replay_record_.result))
                        .arg(QString::fromStdString(replay_record_.reason))
                    : "No replay loaded.");
        } else if (live_review_ply_) {
            status_->setText(
                displayed.white_to_move()
                    ? "Reviewing — White to move in this position"
                    : "Reviewing — Black to move in this position");
        } else if (game_.finished()) {
            status_->setText(result_text());
        } else {
            QString text =
                game_.white_to_move() ? "White to move" : "Black to move";

            if (game_.in_check()) text += " — check";

            if (mode_ == Mode::AI || mode_ == Mode::Random) {
                text += game_.white_to_move() == human_white_
                    ? " — your turn"
                    : mode_ == Mode::AI
                        ? " — AI thinking"
                        : " — random opponent";
            }

            if (mode_ == Mode::AI) {
                text += QString(" — %1 CPU worker(s)").arg(limits_.workers);

                if (game_.white_to_move() == human_white_ &&
                    ponder_worker_.valid() &&
                    ponder_generation_ &&
                    *ponder_generation_ == generation_) {
                    text += " — AI pondering";
                }

                if (!last_search_.isEmpty()) {
                    text += "\n" + last_search_;
                }
            }

            status_->setText(text);
        }

        refresh_live_review_controls();
        sync_annotations();
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    try {
        ChessWindow window;
        window.show();
        return app.exec();
    } catch (const std::exception& error) {
        QMessageBox::critical(
            nullptr,
            "Could not start ZergChess",
            QString::fromUtf8(error.what()));
        return 1;
    }
}