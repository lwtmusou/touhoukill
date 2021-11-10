
// TODO: make this a std::vector-like implementation

template <class T>
class QList {
public:
    QList();
    ~QList();
    int length() const;
    void append(const T &elem);
    void prepend(const T &elem);
    bool isEmpty() const;
    bool contains(const T &value) const;
    T first() const;
    T last() const;
    void removeAt(int i);
    int removeAll(const T &value);
    bool removeOne(const T &value);
    QList<T> mid(int pos, int length = -1) const;
    int indexOf(const T &value, int from = 0);
};

%extend QList {
    T at(int i) const{
        return $self->value(i);
    }
}

%template(SPlayerList) QList<Player *>;
%template(PlayerList)  QList<const Player *>;
%template(CardList) QList<const Card *>;
%template(IntList) QList<int>;
%template(SkillList) QList<const Skill *>;
%template(CardsMoveList) QList<CardsMoveStruct>;
%template(PlaceList) QList<QSanguosha::Place>;
%template(PhaseList) QList<QSanguosha::Phase>;
%template(VariantList) QList<QVariant>;

typedef QList<QVariant> QVariantList;
typedef QList<QString> QStringList;
