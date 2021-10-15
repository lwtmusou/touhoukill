#include "distanceviewdialog.h"

#include "client.h"
#include "engine.h"
#include "roomscene.h"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>

class DistanceViewDialogUI
{
public:
    DistanceViewDialogUI()
    {
        from = new QComboBox;
        to = new QComboBox;

        left = new QLineEdit;
        right = new QLineEdit;

        min = new QLineEdit;
        in_attack = new QLineEdit;

        QSet<const DistanceSkill *> skills = ClientInstance->getDistanceSkills();
        foreach (const DistanceSkill *skill, skills) {
            QLineEdit *distance_edit = new QLineEdit;
            distance_edit->setObjectName(skill->objectName());
            distance_edit->setReadOnly(true);
            distance_edits << distance_edit;
        }

        final = new QLineEdit;
        final->setReadOnly(true);
    }

    QComboBox *from, *to;
    QLineEdit *left, *right;
    QLineEdit *min;
    QList<QLineEdit *> distance_edits;
    QLineEdit *in_attack;
    QLineEdit *final;

private:
    Q_DISABLE_COPY(DistanceViewDialogUI)
};

DistanceViewDialog::DistanceViewDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Distance view"));

    QFormLayout *layout = new QFormLayout;

    ui = new DistanceViewDialogUI;

    RoomScene::FillPlayerNames(ui->from, false);
    RoomScene::FillPlayerNames(ui->to, false);

    connect(ui->from, &QComboBox::currentIndexChanged, this, &DistanceViewDialog::showDistance);
    connect(ui->to, &QComboBox::currentIndexChanged, this, &DistanceViewDialog::showDistance);

    layout->addRow(tr("From"), ui->from);
    layout->addRow(tr("To"), ui->to);
    layout->addRow(tr("Left"), ui->left);
    layout->addRow(tr("Right"), ui->right);
    layout->addRow(tr("Minimum"), ui->min);

    QGroupBox *box = new QGroupBox();
    layout->addRow(tr("Distance correct"), box);

    QFormLayout *box_layout = new QFormLayout;
    foreach (QLineEdit *edit, ui->distance_edits)
        box_layout->addRow(Sanguosha->translate(edit->objectName()), edit);

    box->setLayout(box_layout);

    layout->addRow(tr("In attack range"), ui->in_attack);
    layout->addRow(tr("Final"), ui->final);
    setLayout(layout);

    showDistance();
}

DistanceViewDialog::~DistanceViewDialog()
{
    delete ui;
}

void DistanceViewDialog::showDistance()
{
    QString from_name = ui->from->itemData(ui->from->currentIndex()).toString();
    QString to_name = ui->to->itemData(ui->to->currentIndex()).toString();

    const Player *from = ClientInstance->findPlayer(from_name);
    const Player *to = ClientInstance->findPlayer(to_name);

    if (from->isRemoved() || to->isRemoved()) {
        ui->right->setText(tr("Not exist"));
        ui->left->setText(tr("Not exist"));
        ui->min->setText(tr("Not exist"));
    } else {
        int right_distance = from->originalRightDistanceTo(to);
        ui->right->setText(QString::number(right_distance));

        int left_distance = ClientInstance->players(false, false).length() - right_distance;
        ui->left->setText(QString::number(left_distance));

        int min = qMin(left_distance, right_distance);
        ui->min->setText(QStringLiteral("min(%1, %2)=%3").arg(left_distance, right_distance, min));
    }

    foreach (QLineEdit *edit, ui->distance_edits) {
        const Skill *skill = Sanguosha->getSkill(edit->objectName());
        const DistanceSkill *distance_skill = qobject_cast<const DistanceSkill *>(skill);
        int correct = distance_skill->getCorrect(from, to);

        if (correct > 0)
            edit->setText(QStringLiteral("+%1").arg(correct));
        else if (correct < 0)
            edit->setText(QString::number(correct));
        else
            edit->setText(QString());
    }

    ui->in_attack->setText(from->inMyAttackRange(to) ? tr("Yes") : tr("No"));

    if (from->isRemoved() || to->isRemoved())
        ui->final->setText(tr("Not exist"));
    else
        ui->final->setText(QString::number(from->distanceTo(to)));
}
