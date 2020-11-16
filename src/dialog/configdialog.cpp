#include "configdialog.h"
#include "audio.h"
#include "engine.h"
#include "settings.h"
#include "ui_configdialog.h"

#include <QColorDialog>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFontDialog>

#include <random>

ConfigDialog::ConfigDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConfigDialog)
{
    ui->setupUi(this);

    // tab 1
    QString bg_path = Config.value("BackgroundImage").toString();
    if (!bg_path.startsWith(":"))
        ui->bgPathLineEdit->setText(bg_path);

    QString tableBg_path = Config.value("TableBgImage").toString();
    if (!tableBg_path.startsWith(":"))
        ui->tableBgPathLineEdit->setText(tableBg_path);

    ui->bgMusicPathLineEdit->setText(Config.value("BackgroundMusic", "audio/title/main.ogg").toString());

    ui->enableEffectCheckBox->setChecked(Config.EnableEffects);

    ui->enableLastWordCheckBox->setEnabled(Config.EnableEffects);
    ui->enableLastWordCheckBox->setChecked(Config.EnableLastWord);
    connect(ui->enableEffectCheckBox, SIGNAL(toggled(bool)), ui->enableLastWordCheckBox, SLOT(setEnabled(bool)));

    ui->enableBgMusicCheckBox->setChecked(Config.EnableBgMusic);
    ui->UseLordBGMBox->setChecked(Config.UseLordBGM);
    ui->noIndicatorCheckBox->setChecked(Config.value("NoIndicator", false).toBool());
    ui->noEquipAnimCheckBox->setChecked(Config.value("NoEquipAnim", false).toBool());
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

    connect(this, SIGNAL(accepted()), this, SLOT(saveConfig()));

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
    lineedit->setText(QString("%1 %2").arg(font.family()).arg(font.pointSize()));
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::on_browseBgButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Select a background image"), "backdrop/", tr("Images (*.png *.bmp *.jpg)"));

    if (!filename.isEmpty()) {
        ui->bgPathLineEdit->setText(filename);

        Config.BackgroundImage = filename;
        Config.setValue("BackgroundImage", filename);

        emit bg_changed();
    }
}

void ConfigDialog::on_resetBgButton_clicked()
{
    ui->bgPathLineEdit->clear();

    int length = 8;
    int index = QRandomGenerator::global()->generate() % length + 1;
    QString filename = QString("%1%2%3").arg("backdrop/new-version").arg(index).arg(".jpg");

    Config.BackgroundImage = filename;
    Config.setValue("BackgroundImage", filename);

    emit bg_changed();

    Config.remove("BackgroundImage");
}

void ConfigDialog::on_browseTableBgButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Select a tableBg image"), "backdrop/", tr("Images (*.png *.bmp *.jpg)"));

    if (!filename.isEmpty()) {
        ui->tableBgPathLineEdit->setText(filename);

        Config.TableBgImage = filename;
        Config.setValue("TableBgImage", filename);

        emit tableBg_changed();
    }
}

void ConfigDialog::on_resetTableBgButton_clicked()
{
    ui->tableBgPathLineEdit->clear();

    QString filename = "backdrop/default.jpg";
    Config.TableBgImage = filename;
    Config.setValue("TableBgImage", filename);

    emit tableBg_changed();
}

void ConfigDialog::on_browseRecordPathButton_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Select a Record Paths"), "records/");

    if (!path.isEmpty() && ui->recordPathSetupLineEdit->text() != path) {
        ui->recordPathSetupLineEdit->setText(path);
        Config.RecordSavePath = path;
    }
}

void ConfigDialog::on_resetRecordPathButton_clicked()
{
    ui->recordPathSetupLineEdit->clear();

    QString path = "records/";
    if (ui->recordPathSetupLineEdit->text() != path) {
        ui->recordPathSetupLineEdit->setText(path);
        Config.RecordSavePath = path;
    }
}

void ConfigDialog::saveConfig()
{
    float volume = ui->bgmVolumeSlider->value() / 100.0;
    Config.BGMVolume = volume;
    Config.setValue("BGMVolume", volume);
    volume = ui->effectVolumeSlider->value() / 100.0;
    Config.EffectVolume = volume;
    Config.setValue("EffectVolume", volume);
#ifdef AUDIO_SUPPORT
    Audio::setBGMVolume(Config.BGMVolume);
#endif
    bool enabled = ui->enableEffectCheckBox->isChecked();
    Config.EnableEffects = enabled;
    Config.setValue("EnableEffects", enabled);

    enabled = ui->enableLastWordCheckBox->isChecked();
    Config.EnableLastWord = enabled;
    Config.setValue("EnableLastWord", enabled);

    enabled = ui->enableBgMusicCheckBox->isChecked();
    Config.EnableBgMusic = enabled;
    Config.setValue("EnableBgMusic", enabled);

    enabled = ui->UseLordBGMBox->isChecked();
    Config.UseLordBGM = enabled;
    Config.setValue("UseLordBGM", enabled);

    enabled = ui->UseLordBackdropBox->isChecked();
    Config.UseLordBackdrop = enabled;
    Config.setValue("UseLordBackdrop", enabled);

    Config.setValue("NoIndicator", ui->noIndicatorCheckBox->isChecked());
    Config.setValue("NoEquipAnim", ui->noEquipAnimCheckBox->isChecked());

    Config.NeverNullifyMyTrick = ui->neverNullifyMyTrickCheckBox->isChecked();
    Config.setValue("NeverNullifyMyTrick", Config.NeverNullifyMyTrick);

    Config.EnableAutoTarget = ui->autoTargetCheckBox->isChecked();
    Config.setValue("EnableAutoTarget", Config.EnableAutoTarget);

    Config.EnableIntellectualSelection = ui->intellectualSelectionCheckBox->isChecked();
    Config.setValue("EnableIntellectualSelection", Config.EnableIntellectualSelection);

    Config.EnableDoubleClick = ui->doubleClickCheckBox->isChecked();
    Config.setValue("EnableDoubleClick", Config.EnableDoubleClick);

    Config.DefaultHeroSkin = ui->defaultHeroskinCheckBox->isChecked();
    Config.setValue("DefaultHeroSkin", Config.DefaultHeroSkin);

    Config.BubbleChatBoxDelaySeconds = ui->bubbleChatBoxDelaySpinBox->value();
    Config.setValue("BubbleChatBoxDelaySeconds", Config.BubbleChatBoxDelaySeconds);

    Config.EnableAutoSaveRecord = ui->enableAutoSaveCheckBox->isChecked();
    Config.setValue("EnableAutoSaveRecord", Config.EnableAutoSaveRecord);

    Config.NetworkOnly = ui->networkOnlyCheckBox->isChecked();
    Config.setValue("NetworkOnly", Config.NetworkOnly);

    Config.setValue("RecordSavePath", Config.RecordSavePath);

    Config.setValue("EnableAutoUpdate", ui->autoUpdateCheckBox->isChecked());
    Config.EnableAutoUpdate = ui->autoUpdateCheckBox->isChecked();
}

void ConfigDialog::on_browseBgMusicButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Select a background music"), "audio/system", tr("Audio files (*.wav *.mp3 *.ogg)"));
    if (!filename.isEmpty()) {
        ui->bgMusicPathLineEdit->setText(filename);
        Config.setValue("BackgroundMusic", filename);
    }
}

void ConfigDialog::on_resetBgMusicButton_clicked()
{
    QString default_music = "audio/title/main.ogg";
    Config.setValue("BackgroundMusic", default_music);
    ui->bgMusicPathLineEdit->setText(default_music);
}

void ConfigDialog::on_changeAppFontButton_clicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, Config.AppFont, this);
    if (ok) {
        Config.AppFont = font;
        showFont(ui->appFontLineEdit, font);

        Config.setValue("AppFont", font);
        QApplication::setFont(font);
    }
}

void ConfigDialog::on_setTextEditFontButton_clicked()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, Config.UIFont, this);
    if (ok) {
        Config.UIFont = font;
        showFont(ui->textEditFontLineEdit, font);

        Config.setValue("UIFont", font);
        QApplication::setFont(font, "QTextEdit");
    }
}

void ConfigDialog::on_setTextEditColorButton_clicked()
{
    QColor color = QColorDialog::getColor(Config.TextEditColor, this);
    if (color.isValid()) {
        Config.TextEditColor = color;
        Config.setValue("TextEditColor", color);
        QPalette palette;
        palette.setColor(QPalette::Text, color);
        int aver = (color.red() + color.green() + color.blue()) / 3;
        palette.setColor(QPalette::Base, aver >= 208 ? Qt::black : Qt::white);
        ui->textEditFontLineEdit->setPalette(palette);
    }
}
