#include "configdialog.h"
#include "audio.h"
#include "engine.h"
#include "settings.h"
#include "ui_configdialog.h"

#include <QColorDialog>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFontDialog>
#include <QRandomGenerator>

#include <random>

ConfigDialog::ConfigDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConfigDialog)
{
    ui->setupUi(this);

    // tab 1
    QString bg_path = Config.value(QStringLiteral("BackgroundImage")).toString();
    if (!bg_path.startsWith(QStringLiteral(":")))
        ui->bgPathLineEdit->setText(bg_path);

    QString tableBg_path = Config.value(QStringLiteral("TableBgImage")).toString();
    if (!tableBg_path.startsWith(QStringLiteral(":")))
        ui->tableBgPathLineEdit->setText(tableBg_path);

    ui->bgMusicPathLineEdit->setText(Config.value(QStringLiteral("BackgroundMusic"), QStringLiteral("audio/title/main.ogg")).toString());

    ui->enableEffectCheckBox->setChecked(Config.EnableEffects);

    ui->enableLastWordCheckBox->setEnabled(Config.EnableEffects);
    ui->enableLastWordCheckBox->setChecked(Config.EnableLastWord);
    connect(ui->enableEffectCheckBox, &QAbstractButton::toggled, ui->enableLastWordCheckBox, &QWidget::setEnabled);

    ui->enableBgMusicCheckBox->setChecked(Config.EnableBgMusic);
    ui->UseLordBGMBox->setChecked(Config.UseLordBGM);
    ui->noIndicatorCheckBox->setChecked(Config.value(QStringLiteral("NoIndicator"), false).toBool());
    ui->noEquipAnimCheckBox->setChecked(Config.value(QStringLiteral("NoEquipAnim"), false).toBool());
    ui->UseLordBackdropBox->setChecked(Config.UseLordBackdrop);

    ui->bgmVolumeSlider->setValue(100 * Config.BGMVolume);
    ui->effectVolumeSlider->setValue(100 * Config.EffectVolume);

    // tab 2
    ui->neverNullifyMyTrickCheckBox->setChecked(Config.NeverNullifyMyTrick);
    ui->autoTargetCheckBox->setChecked(Config.EnableAutoTarget);
    ui->intellectualSelectionCheckBox->setChecked(Config.EnableIntellectualSelection);
    ui->defaultHeroskinCheckBox->setChecked(Config.DefaultHeroSkin);
    ui->doubleClickCheckBox->setChecked(Config.EnableDoubleClick);
    ui->autoUpdateCheckBox->setChecked(Config.EnableAutoUpdate);
    ui->bubbleChatBoxDelaySpinBox->setSuffix(tr(" second"));
    ui->bubbleChatBoxDelaySpinBox->setValue(Config.BubbleChatBoxDelaySeconds);

    connect(this, &QDialog::accepted, this, &ConfigDialog::saveConfig);

    QFont font = Config.AppFont;
    showFont(ui->appFontLineEdit, font);

    font = Config.UIFont;
    showFont(ui->textEditFontLineEdit, font);

    QPalette palette;
    palette.setColor(QPalette::Text, Config.TextEditColor);
    QColor color = Config.TextEditColor;
    int aver = (color.red() + color.green() + color.blue()) / 3;
    palette.setColor(QPalette::Base, aver >= 208 ? Qt::black : Qt::white);
    ui->textEditFontLineEdit->setPalette(palette);

    ui->enableAutoSaveCheckBox->setChecked(Config.EnableAutoSaveRecord);
    ui->networkOnlyCheckBox->setChecked(Config.NetworkOnly);

    ui->networkOnlyCheckBox->setEnabled(ui->enableAutoSaveCheckBox->isChecked());
    ui->recordPathSetupLabel->setEnabled(ui->enableAutoSaveCheckBox->isChecked());
    ui->recordPathSetupLineEdit->setEnabled(ui->enableAutoSaveCheckBox->isChecked());
    ui->browseRecordPathButton->setEnabled(ui->enableAutoSaveCheckBox->isChecked());
    ui->resetRecordPathButton->setEnabled(ui->enableAutoSaveCheckBox->isChecked());

    connect(ui->enableAutoSaveCheckBox, &QCheckBox::toggled, ui->networkOnlyCheckBox, &QCheckBox::setEnabled);
    connect(ui->enableAutoSaveCheckBox, &QCheckBox::toggled, ui->recordPathSetupLabel, &QLabel::setEnabled);
    connect(ui->enableAutoSaveCheckBox, &QCheckBox::toggled, ui->recordPathSetupLineEdit, &QLineEdit::setEnabled);
    connect(ui->enableAutoSaveCheckBox, &QCheckBox::toggled, ui->browseRecordPathButton, &QPushButton::setEnabled);
    connect(ui->enableAutoSaveCheckBox, &QCheckBox::toggled, ui->resetRecordPathButton, &QPushButton::setEnabled);

    ui->recordPathSetupLineEdit->setText(Config.RecordSavePath);
}

void ConfigDialog::showFont(QLineEdit *lineedit, const QFont &font)
{
    lineedit->setFont(font);
    lineedit->setText(QStringLiteral("%1 %2").arg(font.family(), font.pointSize()));
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::on_browseBgButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Select a background image"), QStringLiteral("backdrop/"), tr("Images (*.png *.bmp *.jpg)"));

    if (!filename.isEmpty()) {
        ui->bgPathLineEdit->setText(filename);

        Config.BackgroundImage = filename;
        Config.setValue(QStringLiteral("BackgroundImage"), filename);

        emit bg_changed();
    }
}

void ConfigDialog::on_resetBgButton_clicked()
{
    ui->bgPathLineEdit->clear();

    int length = 8;
    int index = QRandomGenerator::global()->generate() % length + 1;
    QString filename = QStringLiteral("%1%2%3").arg(QStringLiteral("backdrop/new-version"), QString::number(index), QStringLiteral(".jpg"));

    Config.BackgroundImage = filename;
    Config.setValue(QStringLiteral("BackgroundImage"), filename);

    emit bg_changed();

    Config.remove(QStringLiteral("BackgroundImage"));
}

void ConfigDialog::on_browseTableBgButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Select a tableBg image"), QStringLiteral("backdrop/"), tr("Images (*.png *.bmp *.jpg)"));

    if (!filename.isEmpty()) {
        ui->tableBgPathLineEdit->setText(filename);

        Config.TableBgImage = filename;
        Config.setValue(QStringLiteral("TableBgImage"), filename);

        emit tableBg_changed();
    }
}

void ConfigDialog::on_resetTableBgButton_clicked()
{
    ui->tableBgPathLineEdit->clear();

    QString filename = QStringLiteral("backdrop/default.jpg");
    Config.TableBgImage = filename;
    Config.setValue(QStringLiteral("TableBgImage"), filename);

    emit tableBg_changed();
}

void ConfigDialog::on_browseRecordPathButton_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Select a Record Paths"), QStringLiteral("records/"));

    if (!path.isEmpty() && ui->recordPathSetupLineEdit->text() != path) {
        ui->recordPathSetupLineEdit->setText(path);
        Config.RecordSavePath = path;
    }
}

void ConfigDialog::on_resetRecordPathButton_clicked()
{
    ui->recordPathSetupLineEdit->clear();

    QString path = QStringLiteral("records/");
    if (ui->recordPathSetupLineEdit->text() != path) {
        ui->recordPathSetupLineEdit->setText(path);
        Config.RecordSavePath = path;
    }
}

void ConfigDialog::saveConfig()
{
    float volume = ui->bgmVolumeSlider->value() / 100.0;
    Config.BGMVolume = volume;
    Config.setValue(QStringLiteral("BGMVolume"), volume);
    volume = ui->effectVolumeSlider->value() / 100.0;
    Config.EffectVolume = volume;
    Config.setValue(QStringLiteral("EffectVolume"), volume);

    Audio::setBGMVolume(Config.BGMVolume);

    bool enabled = ui->enableEffectCheckBox->isChecked();
    Config.EnableEffects = enabled;
    Config.setValue(QStringLiteral("EnableEffects"), enabled);

    enabled = ui->enableLastWordCheckBox->isChecked();
    Config.setValue(QStringLiteral("EnableLastWord"), enabled);

    enabled = ui->enableBgMusicCheckBox->isChecked();
    Config.EnableBgMusic = enabled;
    Config.setValue(QStringLiteral("EnableBgMusic"), enabled);

    enabled = ui->UseLordBGMBox->isChecked();
    Config.UseLordBGM = enabled;
    Config.setValue(QStringLiteral("UseLordBGM"), enabled);

    enabled = ui->UseLordBackdropBox->isChecked();
    Config.setValue(QStringLiteral("UseLordBackdrop"), enabled);

    Config.setValue(QStringLiteral("NoIndicator"), ui->noIndicatorCheckBox->isChecked());
    Config.setValue(QStringLiteral("NoEquipAnim"), ui->noEquipAnimCheckBox->isChecked());

    Config.NeverNullifyMyTrick = ui->neverNullifyMyTrickCheckBox->isChecked();
    Config.setValue(QStringLiteral("NeverNullifyMyTrick"), Config.NeverNullifyMyTrick);

    Config.EnableAutoTarget = ui->autoTargetCheckBox->isChecked();
    Config.setValue(QStringLiteral("EnableAutoTarget"), Config.EnableAutoTarget);

    Config.EnableIntellectualSelection = ui->intellectualSelectionCheckBox->isChecked();
    Config.setValue(QStringLiteral("EnableIntellectualSelection"), Config.EnableIntellectualSelection);

    Config.EnableDoubleClick = ui->doubleClickCheckBox->isChecked();
    Config.setValue(QStringLiteral("EnableDoubleClick"), Config.EnableDoubleClick);

    Config.DefaultHeroSkin = ui->defaultHeroskinCheckBox->isChecked();
    Config.setValue(QStringLiteral("DefaultHeroSkin"), Config.DefaultHeroSkin);

    Config.BubbleChatBoxDelaySeconds = ui->bubbleChatBoxDelaySpinBox->value();
    Config.setValue(QStringLiteral("BubbleChatBoxDelaySeconds"), Config.BubbleChatBoxDelaySeconds);

    Config.EnableAutoSaveRecord = ui->enableAutoSaveCheckBox->isChecked();
    Config.setValue(QStringLiteral("EnableAutoSaveRecord"), Config.EnableAutoSaveRecord);

    Config.NetworkOnly = ui->networkOnlyCheckBox->isChecked();
    Config.setValue(QStringLiteral("NetworkOnly"), Config.NetworkOnly);

    Config.setValue(QStringLiteral("RecordSavePath"), Config.RecordSavePath);

    Config.setValue(QStringLiteral("EnableAutoUpdate"), ui->autoUpdateCheckBox->isChecked());
    Config.EnableAutoUpdate = ui->autoUpdateCheckBox->isChecked();
}

void ConfigDialog::on_browseBgMusicButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Select a background music"), QStringLiteral("audio/system"), tr("Audio files (*.wav *.mp3 *.ogg)"));
    if (!filename.isEmpty()) {
        ui->bgMusicPathLineEdit->setText(filename);
        Config.setValue(QStringLiteral("BackgroundMusic"), filename);
    }
}

void ConfigDialog::on_resetBgMusicButton_clicked()
{
    QString default_music = QStringLiteral("audio/title/main.ogg");
    Config.setValue(QStringLiteral("BackgroundMusic"), default_music);
    ui->bgMusicPathLineEdit->setText(default_music);
}

void ConfigDialog::on_changeAppFontButton_clicked()
{
    bool ok = false;
    QFont font = QFontDialog::getFont(&ok, Config.AppFont, this);
    if (ok) {
        Config.AppFont = font;
        showFont(ui->appFontLineEdit, font);

        Config.setValue(QStringLiteral("AppFont"), font);
        QApplication::setFont(font);
    }
}

void ConfigDialog::on_setTextEditFontButton_clicked()
{
    bool ok = false;
    QFont font = QFontDialog::getFont(&ok, Config.UIFont, this);
    if (ok) {
        Config.UIFont = font;
        showFont(ui->textEditFontLineEdit, font);

        Config.setValue(QStringLiteral("UIFont"), font);
        QApplication::setFont(font, "QTextEdit");
    }
}

void ConfigDialog::on_setTextEditColorButton_clicked()
{
    QColor color = QColorDialog::getColor(Config.TextEditColor, this);
    if (color.isValid()) {
        Config.TextEditColor = color;
        Config.setValue(QStringLiteral("TextEditColor"), color);
        QPalette palette;
        palette.setColor(QPalette::Text, color);
        int aver = (color.red() + color.green() + color.blue()) / 3;
        palette.setColor(QPalette::Base, aver >= 208 ? Qt::black : Qt::white);
        ui->textEditFontLineEdit->setPalette(palette);
    }
}
