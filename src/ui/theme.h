#pragma once

#include <QColor>
#include <QString>

class QApplication;
class QLabel;
class QProgressBar;
class QPushButton;

namespace bmap::theme {

// Switch to the Fusion style. Fusion is used deliberately: it is the only
// style that looks the same on all three platforms, it honours QPalette (so
// no style sheet is needed anywhere), and it is the one style that renders
// dark mode correctly on Windows -- the default windowsvista style always
// falls back to the light palette.
//
// Note what this does NOT do: it never touches the *application* palette.
// QApplication::setPalette() marks the palette as owned by the app, after
// which Qt stops folding system light/dark changes into it. Accents are
// applied per widget instead, and re-applied on QEvent::PaletteChange.
void apply(QApplication* app);

bool isDark();

QColor accent();  // progress
QColor danger();  // the irreversible action (Flash)
QColor muted();   // secondary text

// Accent helpers. Each one is idempotent, so a widget can be re-themed on
// every palette change.
QLabel* makeEyebrow(const QString& text);
void makeMuted(QLabel* label);
void makeAccent(QProgressBar* bar);
void makeDanger(QPushButton* button);

}  // namespace bmap::theme
