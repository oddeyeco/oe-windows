#ifndef MACROS_H
#define MACROS_H

#define REGISTER_METRIC_CHECKER( _checker_class_ )              \
Q_DECLARE_METATYPE( _checker_class_ )                           \
namespace __trash {                                             \
class _registrar_##_checker_class_                              \
{                                                               \
public:                                                         \
    _registrar_##_checker_class_()                              \
    {                                                           \
        qRegisterMetaType<_checker_class_>(#_checker_class_);   \
    }                                                           \
};                                                              \
static _registrar_##_checker_class_ _obj##_checker_class_;      \
}

#define DUMMY_COPY_CTOR(_class_) _class_(_class_ const& other) {Q_ASSERT(false);}
#define DECLARE_MERTIC_CHECKER( _class_ ) \
    public: DUMMY_COPY_CTOR(_class_) private:

#endif // MACROS_H
