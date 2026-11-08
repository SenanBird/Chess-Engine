#include <array>
#include <vector>

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QGridLayout>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>

#include "game.hpp"

class ChessWindow : public QWidget {
public:
    ChessWindow() {
        setWindowTitle("Chess");

        // Find the artwork beside the executable's project directory,
        // even when the program is launched from a different directory.
        assets_ = QDir(QCoreApplication::applicationDirPath()
                       + QStringLiteral("/engine/gui/assets"));

        // Keep match history under engine/data beside the app.
        // Use the same absolute path for saving and for the result popup.
        save_path_ = QDir(QCoreApplication::applicationDirPath())
            .absoluteFilePath(QStringLiteral("engine/data/games.tsv"));

        auto* layout = new QVBoxLayout(this);
        status_ = new QLabel(this);
        layout->addWidget(status_);

        auto* board_layout = new QGridLayout;
        board_layout->setSpacing(0);
        layout->addLayout(board_layout);

        // White sits at the bottom. Rank and file labels identify squares.
        for (int row = 0; row < 8; ++row) {
            board_layout->addWidget(new QLabel(QString::number(8 - row), this),
                                    row, 0, Qt::AlignCenter);

            for (int col = 0; col < 8; ++col) {
                auto* square = new QPushButton(this);
                square->setFixedSize(64, 64);
                square->setIconSize(QSize(52, 52));
                squares_[row][col] = square;
                board_layout->addWidget(square, row, col + 1);

                connect(square, &QPushButton::clicked, this,
                        [this, row, col] { click_square(row, col); });
            }
        }

        const QString files = QStringLiteral("abcdefgh");
        for (int col = 0; col < 8; ++col) {
            board_layout->addWidget(new QLabel(files.mid(col, 1), this),
                                    8, col + 1, Qt::AlignCenter);
        }

        auto* new_game = new QPushButton("New game", this);
        layout->addWidget(new_game);
        connect(new_game, &QPushButton::clicked, this, [this] {
            // Ask before discarding a game that has accepted moves.
            if (started_ && !game_.finished() &&
                QMessageBox::question(this, "New game",
                                      "Discard the current game?") != QMessageBox::Yes) {
                return;
            }
            start_new_game();
        });

        refresh();
    }

private:
    Game game_;
    QDir assets_;
    QString save_path_;
    QLabel* status_ = nullptr;
    std::array<std::array<QPushButton*, 8>, 8> squares_{};
    int selected_row_ = -1;
    int selected_col_ = -1;
    std::vector<GameMove> legal_moves_;
    bool started_ = false;

    void clear_selection() {
        selected_row_ = -1;
        selected_col_ = -1;
        legal_moves_.clear();
    }

    void start_new_game() {
        game_ = Game{};
        started_ = false;
        clear_selection();
        refresh();
    }

    QString result_text() const {
        if (game_.reason() == "checkmate") {
            return game_.result() == "1-0"
                ? QStringLiteral("White wins by checkmate!")
                : QStringLiteral("Black wins by checkmate!");
        }

        return QStringLiteral("Draw: %1.")
            .arg(QString::fromStdString(game_.reason()));
    }

    void show_game_over() {
        // This runs only after the move that finishes the game is accepted.
        const bool saved = game_.save_finished_game(save_path_.toStdString());

        QMessageBox dialog(this);
        dialog.setWindowTitle("Game over");
        dialog.setIcon(saved ? QMessageBox::Information : QMessageBox::Warning);
        dialog.setTextFormat(Qt::PlainText);
        dialog.setText(result_text());

        const QString save_message = saved
            ? QStringLiteral("Game saved to:\n%1").arg(save_path_)
            : QStringLiteral("Could not save the game to:\n%1").arg(save_path_);

        dialog.setInformativeText(
            save_message + QStringLiteral("\n\nClick New game to play again."));

        auto* play_again = dialog.addButton("New game", QMessageBox::AcceptRole);
        auto* view_board = dialog.addButton("View board", QMessageBox::RejectRole);
        dialog.setDefaultButton(view_board);
        dialog.setEscapeButton(view_board);
        dialog.exec();

        if (dialog.clickedButton() == play_again) {
            start_new_game();
        }
    }

    void click_square(int row, int col) {
        if (game_.finished()) {
            return;
        }

        // Collect matching moves. Promotions have one move per piece choice.
        std::vector<GameMove> choices;
        for (const auto& move : legal_moves_) {
            if (move.to_row == row && move.to_col == col) {
                choices.push_back(move);
            }
        }

        if (!choices.empty()) {
            GameMove move = choices.front();
            if (move.promotion != '\0') {
                bool confirmed = false;
                const QStringList labels{"Queen", "Rook", "Bishop", "Knight"};
                const QString choice = QInputDialog::getItem(
                    this, "Pawn promotion", "Promote to:", labels, 0, false, &confirmed);
                const auto index = labels.indexOf(choice);
                if (!confirmed || index < 0) {
                    return;
                }
                move.promotion = "qrbn"[index];
            }

            // Refresh before opening the result popup so the final move is visible.
            const bool played = game_.play(move);
            if (played) {
                started_ = true;
            }
            clear_selection();
            refresh();

            if (played && game_.finished()) {
                show_game_over();
            }
            return;
        }

        // Clicking one of your pieces selects it. Clicking it again deselects it.
        const bool clicked_selection = row == selected_row_ && col == selected_col_;
        clear_selection();

        const Board& board = game_.board();
        const Piece* piece = board.get_piece(row, col, board);
        if (!clicked_selection && piece && piece->is_white == game_.white_to_move()) {
            selected_row_ = row;
            selected_col_ = col;
            legal_moves_ = game_.legal_moves_from(row, col);
        }
        refresh();
    }

    void draw_piece(QPushButton* square, const Piece* piece) {
        // Clear the previous icon and label when a piece leaves the square.
        square->setIcon(QIcon());
        square->setText(QString());
        if (!piece) {
            return;
        }

        // Convert "Knight" and White into "knight-w.svg".
        const QString name = QString::fromStdString(piece->name).toLower();
        const QString suffix = piece->is_white
            ? QStringLiteral("-w.svg") : QStringLiteral("-b.svg");
        const QIcon icon(assets_.filePath(name + suffix));

        // Fall back to the existing label if Qt cannot render the artwork.
        if (icon.pixmap(square->iconSize()).isNull()) {
            square->setText(QString::fromStdString(piece->symbol));
        } else {
            square->setIcon(icon);
        }
    }

    void refresh() {
        const Board& board = game_.board();
        for (int row = 0; row < 8; ++row) {
            for (int col = 0; col < 8; ++col) {
                auto* square = squares_[row][col];
                draw_piece(square, board.get_piece(row, col, board));

                // Tint legal destinations and give the selected square its own color.
                const char* color = (row + col) % 2 ? "#91a873" : "#eee8d5";
                for (const auto& move : legal_moves_) {
                    if (move.to_row == row && move.to_col == col) {
                        color = "#a9d983";
                        break;
                    }
                }
                if (row == selected_row_ && col == selected_col_) {
                    color = "#f3c969";
                }

                square->setStyleSheet(QStringLiteral(
                    "background: %1; color: #202522; border: none;"
                    "font-size: 19px; font-weight: bold;")
                    .arg(QString::fromLatin1(color)));
            }
        }

        if (game_.finished()) {
            status_->setText(result_text());
            return;
        }

        QString status = game_.white_to_move() ? "White to move" : "Black to move";
        if (game_.in_check()) {
            status += " — check";
        }
        status_->setText(status);
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    ChessWindow window;
    window.show();
    return app.exec();
}