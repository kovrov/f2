#ifndef PROGRESSINFO_H
#define PROGRESSINFO_H


#include <QObject>
#include <QAtomicInt>
#include <QExplicitlySharedDataPointer>
#include <QVariant>


namespace models {


class ProgressInfo
{
//    Q_GADGET
public:
    ProgressInfo(quint64 serial=0) : d (new ProgressInfo::Data(serial, -1,1)) {}
    ProgressInfo(const ProgressInfo &other) : d (other.d.data()) {}
    inline ~ProgressInfo() {}  // not really empty

    ProgressInfo &operator=(const ProgressInfo &other) { d = other.d.data(); return *this; }
    operator QVariant() const { return QVariant::fromValue(*this); }
    // disabling some operators
    bool operator==(const ProgressInfo &) const = delete;
    bool operator!=(const ProgressInfo &) const = delete;

    quint64 serial() const { return d->serial; }
    int value() const { return d->value; }
    int total() const { return d->total; }
    void setInfo(quint64 serial, int value=-1, int total=1)
    {
        detach();
        d->serial = serial;
        d->value = value;
        d->total = total;
    }

private:
    inline void detach() { if (d->ref > 1) d.detach(); }

    struct Data
    {
        QAtomicInt ref;
        quint64 serial;
        int value;
        int total;

        Data(quint64 s, int v, int t) : serial(s), value(v), total(t) {}
    private:
        Data &operator=(const Data &) = delete;
    };

    QExplicitlySharedDataPointer<Data> d;
};


}

Q_DECLARE_METATYPE (models::ProgressInfo);



#endif // PROGRESSINFO_H
