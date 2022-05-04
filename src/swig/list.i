
// TODO: make this a std::vector-like implementation

%rename(at) QList::operator [](int);
template <class T>
class QList {
public:
    QList();
    QList(const QList &);
    QList &operator=(const QList &);
    virtual ~QList();

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
    T value(int i);
    T operator [](int i);
};

#if QT_MAJOR_VERSION==6
template<>
class QList<QString>
{
public:
    QList();
    QList(const QList &);
    QList &operator=(const QList &);
    virtual ~QList();

    int length() const;
    void append(const QString &elem);
    void prepend(const QString &elem);
    bool isEmpty() const;
    bool contains(const QString &value) const;
    QString first() const;
    QString last() const;
    void removeAt(int i);
    int removeAll(const QString &value);
    bool removeOne(const QString &value);
    QList<QString> mid(int pos, int length = -1) const;
    int indexOf(const QString &value, int from = 0);
    QString value(int i);
    QString operator [](int i);
    QString join(const QString &sep) const;
};
#endif

template<class T>
class QListIterator
{
public:
    QListIterator(const QList<T> &container);
    QListIterator &operator=(const QList<T> &container);
    void toFront();
    void toBack();
    bool hasNext() const;
    T next();
    T peekNext() const;
    bool hasPrevious() const;
    T previous();
    T peekPrevious() const;
};

template <class T>
class QSet {
public:
    QSet();
    int count() const;
    void insert(const T &elem);
    bool isEmpty() const;
    bool contains(const T &elem) const;
    bool remove(const T &elem);
    QList<T> values() const;
};

template <class T>
class QSetIterator
{
public:
    QSetIterator(const QSet<T> &container);
    QSetIterator &operator=(const QSet<T> &container);
    void toFront();
    void toBack();
    bool hasNext() const;
    T next();
    T peekNext() const;
    bool hasPrevious() const;
};

%newobject ::create_qlist_iterator;
%newobject ::create_qset_iterator;
%{
template<class T> QListIterator<T> *create_qlist_iterator(QList<T> *list)
{
    return new QListIterator<T>(*list);
}
template<class T> QSetIterator<T> *create_qset_iterator(QSet<T> *set)
{
    return new QSetIterator<T>(*set);
}
%}

template<class T>
QListIterator<T> *create_qlist_iterator(QList<T> *list);

template<class T>
QSetIterator<T> *create_qset_iterator(QSet<T> *list);

%template(SPlayerList) QList<Player *>;
%template(PlayerList) QList<const Player *>;
%template(CardList) QList<const Card *>;
%template(IntList) QList<int>;
%template(StringList) QList<QString>;
%template(PlaceList) QList<QSanguosha::Place>;
%template(PhaseList) QList<QSanguosha::Phase>;
%template(VariantList) QList<QVariant>;
%template(SingleCardMoveList) QList<SingleCardMoveStruct>;

%template(SPlayerListIt) QListIterator<Player *>;
%template(PlayerListIt) QListIterator<const Player *>;
%template(CardListIt) QListIterator<const Card *>;
%template(IntListIt) QListIterator<int>;
%template(StringListIt) QListIterator<QString>;
%template(PlaceListIt) QListIterator<QSanguosha::Place>;
%template(PhaseListIt) QListIterator<QSanguosha::Phase>;
%template(VariantListIt) QListIterator<QVariant>;
%template(SingleCardMoveListIt) QListIterator<SingleCardMoveStruct>;

%template(create_qlist_iterator) create_qlist_iterator<Player *>;
%template(create_qlist_iterator) create_qlist_iterator<const Player *>;
%template(create_qlist_iterator) create_qlist_iterator<const Card *>;
%template(create_qlist_iterator) create_qlist_iterator<int>;
%template(create_qlist_iterator) create_qlist_iterator<QString>;
%template(create_qlist_iterator) create_qlist_iterator<QSanguosha::Place>;
%template(create_qlist_iterator) create_qlist_iterator<QSanguosha::Phase>;
%template(create_qlist_iterator) create_qlist_iterator<QVariant>;
%template(create_qlist_iterator) create_qlist_iterator<SingleCardMoveStruct>;

%template(CardIdSet) QSet<int>;
%template(TriggerEventSet) QSet<QSanguosha::TriggerEvent>;
%template(StringSet) QSet<QString>;
%template(SkillSet) QSet<const Skill *>;
%template(CardSet) QSet<Card *>;
%template(ConstCardSet) QSet<const Card *>;
%template(TriggerSet) QSet<const Trigger *>;

%template(CardIdSetIt) QSetIterator<int>;
%template(TriggerEventSetIt) QSetIterator<QSanguosha::TriggerEvent>;
%template(StringSetIt) QSetIterator<int>;
%template(SkillSetIt) QSetIterator<const Skill *>;
%template(CardSetIt) QSetIterator<Card *>;
%template(ConstCardSetIt) QSetIterator<const Card *>;
%template(TriggerSetIt) QSetIterator<const Trigger *>;

%template(create_qlist_iterator) create_qset_iterator<int>;
%template(create_qlist_iterator) create_qset_iterator<QSanguosha::TriggerEvent>;
%template(create_qlist_iterator) create_qset_iterator<QString>;
%template(create_qlist_iterator) create_qset_iterator<const Skill *>;
%template(create_qlist_iterator) create_qset_iterator<Card *>;
%template(create_qlist_iterator) create_qset_iterator<const Card *>;
%template(create_qlist_iterator) create_qset_iterator<const Trigger *>;

typedef QList<QVariant> QVariantList;
typedef QSet<int> IdSet;

#if QT_MAJOR_VERSION==6
typedef QList<QString> QStringList;
#else
class QStringList: public QList<QString>
{
public:
    QStringList(const QList<QString> &);
    QStringList(const QStringList &);
    ~QStringList() override;

    QStringList &operator=(const QList<QString> &);
    QStringList &operator=(const QStringList &);

    QString join(const QString &sep) const;
};
#endif
