
// TODO: make this a std::vector-like implementation

%rename(at) QList::operator [](int);
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
    T value(int i);
    T operator [](int i);
};

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
    ~QSet();
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
%template(SkillList) QList<const Skill *>;
%template(PlaceList) QList<QSanguosha::Place>;
%template(PhaseList) QList<QSanguosha::Phase>;
%template(VariantList) QList<QVariant>;
%template(SingleCardMoveList) QList<SingleCardMoveStruct>;

%template(SPlayerListIt) QListIterator<Player *>;
%template(PlayerListIt) QListIterator<const Player *>;
%template(CardListIt) QListIterator<const Card *>;
%template(IntListIt) QListIterator<int>;
%template(StringListIt) QListIterator<QString>;
%template(SkillListIt) QListIterator<const Skill *>;
%template(PlaceListIt) QListIterator<QSanguosha::Place>;
%template(PhaseListIt) QListIterator<QSanguosha::Phase>;
%template(VariantListIt) QListIterator<QVariant>;
%template(SingleCardMoveListIt) QListIterator<SingleCardMoveStruct>;

%template(create_qlist_iterator) create_qlist_iterator<Player *>;
%template(create_qlist_iterator) create_qlist_iterator<const Player *>;
%template(create_qlist_iterator) create_qlist_iterator<const Card *>;
%template(create_qlist_iterator) create_qlist_iterator<int>;
%template(create_qlist_iterator) create_qlist_iterator<QString>;
%template(create_qlist_iterator) create_qlist_iterator<const Skill *>;
%template(create_qlist_iterator) create_qlist_iterator<QSanguosha::Place>;
%template(create_qlist_iterator) create_qlist_iterator<QSanguosha::Phase>;
%template(create_qlist_iterator) create_qlist_iterator<QVariant>;
%template(create_qlist_iterator) create_qlist_iterator<SingleCardMoveStruct>;

%template(CardIdSet) QSet<int>;
%template(StringSet) QSet<QString>;

%template(CardIdSetIt) QSetIterator<int>;
%template(StringSetIt) QSetIterator<int>;

%template(create_qlist_iterator) create_qset_iterator<int>;
%template(create_qlist_iterator) create_qset_iterator<QString>;

typedef QList<QVariant> QVariantList;
typedef QSet<int> IdSet;

class QStringList: public QList<QString>
{
public:
    QString join(const QString &sep) const;
};