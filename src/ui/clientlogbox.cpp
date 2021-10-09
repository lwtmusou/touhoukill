#include "clientlogbox.h"
#include "client.h"
#include "engine.h"
#include "roomscene.h"
#include "settings.h"

#include <QPalette>

ClientLogBox::ClientLogBox(QWidget *parent)
    : QTextEdit(parent)
{
    setReadOnly(true);
}

void ClientLogBox::appendLog(const QString &type, const QString &from_general, const QStringList &tos, const QString &card_str, QString arg, QString arg2)
{
    if (Self->hasFlag(QStringLiteral("marshalling")))
        return;

    if (type == QStringLiteral("$AppendSeparator")) {
        append(QString(tr("<font color='%1'>------------------------------</font>")).arg(Config.TextEditColor.name()));
        return;
    }

    QString from;
    if (!from_general.isEmpty()) {
        from = ClientInstance->getPlayerName(from_general);
        from = bold(from, Qt::green);
    }

    QString to;
    if (!tos.isEmpty()) {
        QStringList to_list;
        foreach (QString to, tos)
            to_list << ClientInstance->getPlayerName(to);
        to = to_list.join(QStringLiteral(", "));
        to = bold(to, Qt::red);
    }

    QString log;

    if (type.startsWith(QStringLiteral("$"))) {
        QString log_name;
        foreach (QString one_card, card_str.split(QStringLiteral("+"))) {
            if (type == QStringLiteral("$JudgeResult") || type == QStringLiteral("$PasteCard")) {
                const Card *card = ClientInstance->getCard(one_card.toInt());
                if (card != nullptr) {
                    if (log_name.isEmpty())
                        log_name = card->logName();
                    else
                        log_name += QStringLiteral(", ") + card->logName();
                }
            } else {
                const CardDescriptor &card = Sanguosha->getEngineCard(one_card.toInt());
                if (card.face() != nullptr) {
                    if (log_name.isEmpty())
                        log_name = card.logName();
                    else
                        log_name += QStringLiteral(", ") + card.logName();
                }
            }
        }
        log_name = bold(log_name, Qt::yellow);

        log = Sanguosha->translate(type);
        log.replace(QStringLiteral("%from"), from);
        log.replace(QStringLiteral("%to"), to);
        log.replace(QStringLiteral("%card"), log_name);

        if (!arg2.isEmpty()) {
            arg2 = bold(Sanguosha->translate(arg2), Qt::yellow);
            log.replace(QStringLiteral("%arg2"), arg2);
        }

        if (!arg.isEmpty()) {
            arg = bold(Sanguosha->translate(arg), Qt::yellow);
            log.replace(QStringLiteral("%arg"), arg);
        }

        log = QStringLiteral("<font color='%2'>%1</font>").arg(log).arg(Config.TextEditColor.name());
        append(log);

        return;
    }

    if (!card_str.isEmpty() && !from_general.isEmpty()) {
        // do Indicator animation
        foreach (QString to, tos)
            RoomSceneInstance->showIndicator(from_general, to);

        const Card *card = Card::Parse(card_str, ClientInstance);
        if (card == nullptr)
            return;

        QString card_name = card->logName();
        card_name = bold(card_name, Qt::yellow);

        QString reason = tr("using");
        if (type.endsWith(QStringLiteral("_Resp")))
            reason = tr("playing");
        if (type.endsWith(QStringLiteral("_Recast")))
            reason = tr("recasting");

        if (card->isVirtualCard()) {
            QString skill_name = Sanguosha->translate(card->skillName());
            skill_name = bold(skill_name, Qt::yellow);
            bool eff = (card->skillName(false) != card->skillName(true));
            QString meth = eff ? tr("carry out") : tr("use skill");
            QString suffix = eff ? tr("effect") : QString();

            IDSet card_ids = card->subcards();
            QStringList subcard_list;
            foreach (int card_id, card_ids) {
                const CardDescriptor &subcard = Sanguosha->getEngineCard(card_id);
                subcard_list << bold(subcard.logName(), Qt::yellow);
            }

            QString subcard_str = subcard_list.join(QStringLiteral(", "));
            if (card->face()->type() == QSanguosha::TypeSkill) {
                if (subcard_list.isEmpty() || !card->throwWhenUsing())
                    log = tr("%from %2 [%1] %3").arg(skill_name).arg(meth).arg(suffix);
                else
                    log = tr("%from %3 [%1] %4, and the cost is %2").arg(skill_name).arg(subcard_str).arg(meth).arg(suffix);
            } else {
                if (subcard_list.isEmpty() || card->skillName().contains(QStringLiteral("guhuo")))
                    log = tr("%from %4 [%1] %5, %3 [%2]").arg(skill_name).arg(card_name).arg(reason).arg(meth).arg(suffix);
                else
                    log = tr("%from %5 [%1] %6 %4 %2 as %3").arg(skill_name).arg(subcard_str).arg(card_name).arg(reason).arg(meth).arg(suffix);
            }

            ClientInstance->cardDeleting(card);
        } else if (!card->skillName().isEmpty()) {
            const CardDescriptor &real = Sanguosha->getEngineCard(card->effectiveID());
            QString skill_name = Sanguosha->translate(card->skillName());
            skill_name = bold(skill_name, Qt::yellow);

            QString subcard_str = bold(real.logName(), Qt::yellow);
            if (card->face()->isKindOf("DelayedTrick"))
                log = tr("%from %5 [%1] %6 %4 %2 as %3").arg(skill_name).arg(subcard_str).arg(card_name).arg(reason).arg(tr("use skill")).arg(QString());
            else
                log = tr("Due to the effect of [%1], %from %4 %2 as %3").arg(skill_name).arg(subcard_str).arg(card_name).arg(reason);
        } else
            log = tr("%from %2 %1").arg(card_name).arg(reason);

        if (!to.isEmpty())
            log.append(tr(", target is %to"));
    } else
        log = Sanguosha->translate(type);

    log.replace(QStringLiteral("%from"), from);
    log.replace(QStringLiteral("%to"), to);

    if (!arg2.isEmpty()) {
        arg2 = bold(Sanguosha->translate(arg2), Qt::yellow);
        log.replace(QStringLiteral("%arg2"), arg2);
    }

    if (!arg.isEmpty()) {
        arg = bold(Sanguosha->translate(arg), Qt::yellow);
        log.replace(QStringLiteral("%arg"), arg);
    }

    log = QStringLiteral("<font color='%2'>%1</font>").arg(log).arg(Config.TextEditColor.name());
    QString final_log = append(log);
    if (type.contains(QStringLiteral("#Guhuo")))
        RoomSceneInstance->setGuhuoLog(final_log);
    else if (type == QStringLiteral("#Chanyuan"))
        RoomSceneInstance->setGuhuoLog(QString());
}

QString ClientLogBox::bold(const QString &str, QColor color) const
{
    return QStringLiteral("<font color='%1'><b>%2</b></font>").arg(color.name()).arg(str);
}

void ClientLogBox::appendLog(const QStringList &log_str)
{
    QString err_string = QString();
    if (log_str.length() != 6 || (!log_str.first().startsWith(QStringLiteral("$")) && !log_str.first().startsWith(QStringLiteral("#")))) {
        err_string = tr("Log string is not well formatted: %1").arg(log_str.join(QStringLiteral(",")));
        append(QStringLiteral("<font color='%2'>%1</font>").arg(err_string).arg(Config.TextEditColor.name()));
        return;
    }
    appendLog(log_str[0], log_str[1], log_str[2].isEmpty() ? QStringList() : log_str[2].split(QStringLiteral("+")), log_str[3], log_str[4], log_str[5]);
}

QString ClientLogBox::append(const QString &text)
{
    QString to_append = QStringLiteral("<p style=\"margin:3px 2px; line-height:120%;\">%1</p>").arg(text);
    QTextEdit::append(to_append);
    return to_append;
}
