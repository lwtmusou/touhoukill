class QVariant {
public:
    QVariant();
    QVariant(int);
    QVariant(const char *);
    QVariant(bool);
    int toInt() const;
    QString toString() const;
    QStringList toStringList() const;
    bool toBool() const;
	QList<QVariant> toList() const;
};

%extend QVariant {
    void setValue(int value) {
        $self->setValue(QVariant::fromValue(value));
    }

    void setValue(DamageStruct *damage) {
        $self->setValue(QVariant::fromValue(*damage));
    }

    DamageStruct toDamage() const{
        return $self->value<DamageStruct>();
    }

    void setValue(CardEffectStruct *effect) {
        $self->setValue(QVariant::fromValue(*effect));
    }

    CardEffectStruct toCardEffect() const{
        return $self->value<CardEffectStruct>();
    }

    void setValue(SlashEffectStruct *effect) {
        $self->setValue(QVariant::fromValue(*effect));
    }

    SlashEffectStruct toSlashEffect() const{
        return $self->value<SlashEffectStruct>();
    }

    void setValue(CardUseStruct *use) {
        $self->setValue(QVariant::fromValue(*use));
    }

    CardUseStruct toCardUse() const{
        return $self->value<CardUseStruct>();
    }

    void setValue(const Card *card) {
        $self->setValue(QVariant::fromValue(card));
    }

    const Card *toCard() const{
        return $self->value<const Card *>();
    }

    void setValue(ServerPlayer *player) {
        $self->setValue(QVariant::fromValue(player));
    }

    ServerPlayer *toPlayer() const{
        return $self->value<ServerPlayer *>();
    }

    void setValue(DyingStruct *dying) {
        $self->setValue(QVariant::fromValue(*dying));
    }

    DyingStruct toDying() const{
        return $self->value<DyingStruct>();
    }

    void setValue(DeathStruct *death) {
        $self->setValue(QVariant::fromValue(*death));
    }

    DeathStruct toDeath() const{
        return $self->value<DeathStruct>();
    }

    void setValue(RecoverStruct *recover) {
        $self->setValue(QVariant::fromValue(*recover));
    }

    RecoverStruct toRecover() const{
        return $self->value<RecoverStruct>();
    }

    void setValue(JudgeStruct *judge) {
        $self->setValue(QVariant::fromValue(judge));
    }

    JudgeStruct *toJudge() const{
        return $self->value<JudgeStruct *>();
    }

    void setValue(PindianStruct *pindian) {
        $self->setValue(QVariant::fromValue(pindian));
    }

    PindianStruct *toPindian() const{
        return $self->value<PindianStruct *>();
    }

    void setValue(PhaseChangeStruct *phase) {
        $self->setValue(QVariant::fromValue(*phase));
    }

    PhaseChangeStruct toPhaseChange() const{
        return $self->value<PhaseChangeStruct>();
    }

    void setValue(CardsMoveOneTimeStruct *move) {
        $self->setValue(QVariant::fromValue(*move));
    }

    CardsMoveOneTimeStruct toMoveOneTime() const{
        return $self->value<CardsMoveOneTimeStruct>();
    }

    void setValue(ChoiceMadeStruct *choice) {
        $self->setValue(QVariant::fromValue(*choice));
    }

    ChoiceMadeStruct toChoiceMade() const{
        return $self->value<ChoiceMadeStruct>();
    }

    void setValue(CardAskedStruct *ask) {
        $self->setValue(QVariant::fromValue(*ask));
    }

    CardAskedStruct toCardAsked() const{
        return $self->value<CardAskedStruct>();
    }

    void setValue(CardResponseStruct *resp) {
        $self->setValue(QVariant::fromValue(*resp));
    }

    CardResponseStruct toCardResponse() const{
        return $self->value<CardResponseStruct>();
    }

    void setValue(MarkChangeStruct *change) {
        $self->setValue(QVariant::fromValue(*change));
    }

    MarkChangeStruct toMarkChange() const{
        return $self->value<MarkChangeStruct>();
    }

	void setValue(BrokenEquipChangedStruct *b) {
        $self->setValue(QVariant::fromValue(*b));
    }

    BrokenEquipChangedStruct toBrokenEquipChange() const{
        return $self->value<BrokenEquipChangedStruct>();
    }

	void setValue(ShownCardChangedStruct *s) {
        $self->setValue(QVariant::fromValue(*s));
    }

    ShownCardChangedStruct toShownCardChange() const{
        return $self->value<ShownCardChangedStruct>();
    }

	void setValue(ShowGeneralStruct *s) {
        $self->setValue(QVariant::fromValue(*s));
    }

    ShowGeneralStruct toShowGeneralChange() const{
        return $self->value<ShowGeneralStruct>();
    }

    void setValue(QList<int> intlist) {
        QVariantList varlist;
        for (int i = 0; i < intlist.length(); i++)
            varlist.append(QVariant::fromValue(intlist.at(i)));
        $self->setValue(QVariant::fromValue(varlist));
    }

    QList<int> toIntList() const{
        QList<int> result;
        if ($self->canConvert<QVariantList>()) {
            QVariantList res_var = $self->toList();
            for (int i = 0; i < res_var.length(); i++)
                result.append(res_var.at(i).toInt());
        }
        return result;
    }
};