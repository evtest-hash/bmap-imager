#include "theme.h"

#include <QApplication>
#include <QFont>
#include <QGuiApplication>
#include <QLabel>
#include <QPalette>
#include <QProgressBar>
#include <QPushButton>
#include <QStyleFactory>
#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QStyleHints>
#endif

namespace bmap::theme {

namespace {

// Teal reads as an accent without borrowing any one platform's system blue.
const QColor kAccentLight(0x12, 0x70, 0x7B);
const QColor kAccentDark(0x35, 0xA7, 0xB3);

// Warm: Flash erases the whole device, so it must not look like Cancel.
const QColor kDangerLight(0xB4, 0x54, 0x1F);
const QColor kDangerDark(0xC2, 0x5F, 0x26);

// Scale a font's size by a factor, whichever unit it happens to be set in.
void scaleFont(QFont* f, double factor) {
    if (f->pointSizeF() > 0) {
        f->setPointSizeF(f->pointSizeF() * factor);
    } else if (f->pixelSize() > 0) {
        f->setPixelSize(qMax(9, static_cast<int>(f->pixelSize() * factor)));
    }
}

}  // namespace

void apply(QApplication* app) {
    app->setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
}

bool isDark() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
    // Pre-6.5 has no colorScheme(): infer it from the palette itself.
    const QPalette p = QGuiApplication::palette();
    return p.color(QPalette::WindowText).lightness() >
           p.color(QPalette::Window).lightness();
#endif
}

QColor accent() { return isDark() ? kAccentDark : kAccentLight; }

QColor danger() { return isDark() ? kDangerDark : kDangerLight; }

QColor muted() {
    QColor c = QApplication::palette().color(QPalette::WindowText);
    c.setAlpha(150);
    return c;
}

QLabel* makeEyebrow(const QString& text) {
    auto* label = new QLabel(text.toUpper());
    QFont f = label->font();
    scaleFont(&f, 0.82);
    f.setWeight(QFont::DemiBold);
    f.setLetterSpacing(QFont::PercentageSpacing, 112);
    label->setFont(f);
    makeMuted(label);
    return label;
}

void makeMuted(QLabel* label) {
    QPalette p = label->palette();
    p.setColor(QPalette::WindowText, muted());
    label->setPalette(p);
}

void makeAccent(QProgressBar* bar) {
    // Fusion fills the chunk with the widget's own Highlight role.
    QPalette p = bar->palette();
    p.setColor(QPalette::Highlight, accent());
    bar->setPalette(p);
}

void makeDanger(QPushButton* button) {
    const QColor c = danger();
    QPalette p = button->palette();

    // Set the enabled groups only: a plain setColor() would also paint the
    // Disabled group, leaving the button fully saturated while a flash is
    // running and it is not clickable.
    for (const QPalette::ColorGroup group : {QPalette::Active, QPalette::Inactive}) {
        p.setColor(group, QPalette::Button, c);
        p.setColor(group, QPalette::ButtonText, QColor(Qt::white));
    }
    const QColor off = QColor::fromHsv(c.hue(), c.saturation() / 3, c.value());
    p.setColor(QPalette::Disabled, QPalette::Button, off);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(255, 255, 255, 150));
    button->setPalette(p);

    QFont f = button->font();
    f.setWeight(QFont::DemiBold);
    button->setFont(f);
}

}  // namespace bmap::theme
