#include "clientlogbox.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "roomscene.h"
#include "settings.h"

#include <QPalette>

ClientLogBox::ClientLogBox(QWidget *parent)
    : QTextEdit(parent)
{
    setReadOnly(true);
}

void ClientLogBox::appendLog(const QString &type, const QString &from_general, const QStringList &tos, QString card_str, QString arg, QString arg2)
{
    if (Self->hasFlag("marshalling"))
        return;

    if (type == "$AppendSeparator") {
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
        to = to_list.join(", ");
        to = bold(to, Qt::red);
    }

    QString log;

    if (type.startsWith("$")) {
        QString log_name;
        foreach (QString one_card, card_str.split("+")) {
            if (type == "$JudgeResult" || type == "$PasteCard") {
                const Card *card = ClientInstance->getCard(one_card.toInt());
                if (card) {
                    if (log_name.isEmpty())
                        log_name = card->logName();
                    else
                        log_name += ", " + card->logName();
                }
            } else {
                const CardDescriptor &card = Sanguosha->getEngineCard(one_card.toInt());
                if (card.face != nullptr) {
                    if (log_name.isEmpty())
                        log_name = card.logName();
                    else
                        log_name += ", " + card.logName();
                }
            }
        }
        log_name = bold(log_name, Qt::yellow);

        log = Sanguosha->translate(type);
        log.replace("%from", from);
        log.replace("%to", to);
        log.replace("%card", log_name);

        if (!arg2.isEmpty()) {
            arg2 = bold(Sanguosha->translate(arg2), Qt::yellow);
            log.replace("%arg2", arg2);
        }

        if (!arg.isEmpty()) {
            arg = bold(Sanguosha->translate(arg), Qt::yellow);
            log.replace("%arg", arg);
        }

        log = QString("<font color='%2'>%1</font>").arg(log).arg(Config.TextEditColor.name());
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
        if (type.endsWith("_Resp"))
            reason = tr("playing");
        if (type.endsWith("_Recast"))
            reason = tr("recasting");

        if (card->isVirtualCard()) {
            QString skill_name = Sanguosha->translate(card->skillName());
            skill_name = bold(skill_name, Qt::yellow);
            bool eff = (card->skillName(false) != card->skillName(true));
            QString meth = eff ? tr("carry out") : tr("use skill");
            QString suffix = eff ? tr("effect") : "";

            IDSet card_ids = card->subcards();
            QStringList subcard_list;
            foreach (int card_id, card_ids) {
                const CardDescriptor &subcard = Sanguosha->getEngineCard(card_id);
                subcard_list << bold(subcard.logName(), Qt::yellow);
            }

            QString subcard_str = subcard_list.join(", ");
            if (card->face()->type() == CardFace::TypeSkill) {
                if (subcard_list.isEmpty() || !card->throwWhenUsing())
                    log = tr("%from %2 [%1] %3").arg(skill_name).arg(meth).arg(suffix);
                else
                    log = tr("%from %3 [%1] %4, and the cost is %2").arg(skill_name).arg(subcard_str).arg(meth).arg(suffix);
            } else {
                if (subcard_list.isEmpty() || card->skillName().contains("guhuo"))
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

    log.replace("%from", from);
    log.replace("%to", to);

    if (!arg2.isEmpty()) {
        arg2 = bold(Sanguosha->translate(arg2), Qt::yellow);
        log.replace("%arg2", arg2);
    }

    if (!arg.isEmpty()) {
        arg = bold(Sanguosha->translate(arg), Qt::yellow);
        log.replace("%arg", arg);
    }

    log = QString("<font color='%2'>%1</font>").arg(log).arg(Config.TextEditColor.name());
    QString final_log = append(log);
    if (type.contains("#Guhuo"))
        RoomSceneInstance->setGuhuoLog(final_log);
    else if (type == "#Chanyuan")
        RoomSceneInstance->setGuhuoLog(QString());
}

QString ClientLogBox::bold(const QString &str, QColor color) const
{
    return QString("<font color='%1'><b>%2</b></font>").arg(color.name()).arg(str);
}

void ClientLogBox::appendLog(const QStringList &log_str)
{
    QString err_string = QString();
    if (log_str.length() != 6 || (!log_str.first().startsWith("$") && !log_str.first().startsWith("#"))) {
        err_string = tr("Log string is not well formatted: %1").arg(log_str.join(","));
        append(QString("<font color='%2'>%1</font>").arg(err_string).arg(Config.TextEditColor.name()));
        return;
    }
    appendLog(log_str[0], log_str[1], log_str[2].isEmpty() ? QStringList() : log_str[2].split("+"), log_str[3], log_str[4], log_str[5]);
}

QString ClientLogBox::append(const QString &text)
{
    QString to_append = QString("<p style=\"margin:3px 2px; line-height:120%;\">%1</p>").arg(text);
    QTextEdit::append(to_append);
    return to_append;
}
