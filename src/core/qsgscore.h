#ifndef QSANGUOSHA_QSGSCORE_H
#define QSANGUOSHA_QSGSCORE_H

#include <QtGlobal>

#if defined(SWIG) || !defined(QSGS_STATIC)
#ifdef BUILD_QSGSCORE
#define QSGS_CORE_EXPORT Q_DECL_EXPORT
#else
#define QSGS_CORE_EXPORT Q_DECL_IMPORT
#endif
#else
#define QSGS_CORE_EXPORT
#endif

#endif
