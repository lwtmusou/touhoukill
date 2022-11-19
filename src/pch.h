#ifndef PCH_H
#define PCH_H

#ifndef __cplusplus

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <qglobal.h>

#include <lua.h>

#else

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <functional>
#include <optional>
#include <random>
#include <sstream>
#include <type_traits>

#include <QtGlobal>

#include <Qt>
#include <QtMath>

#include <QAbstractAnimation>
#include <QAbstractButton>
#include <QAbstractListModel>
#include <QAction>
#include <QAnimationGroup>
#include <QApplication>
#include <QAtomicPointer>
#include <QBitmap>
#include <QBoxLayout>
#include <QBrush>
#include <QBuffer>
#include <QButtonGroup>
#include <QByteArray>
#include <QCache>
#include <QCheckBox>
#include <QClipboard>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QCommandLinkButton>
#include <QCompleter>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QCursor>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDrag>
#include <QEasingCurve>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFocusEvent>
#include <QFont>
#include <QFontDatabase>
#include <QFontDialog>
#include <QFontMetrics>
#include <QFormLayout>
#include <QFrame>
#include <QGlobalStatic>
#include <QGraphicsBlurEffect>
#include <QGraphicsColorizeEffect>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsEffect>
#include <QGraphicsItem>
#include <QGraphicsLinearLayout>
#include <QGraphicsObject>
#include <QGraphicsPixmapItem>
#include <QGraphicsProxyWidget>
#include <QGraphicsRectItem>
#include <QGraphicsRotation>
#include <QGraphicsScale>
#include <QGraphicsScene>
#include <QGraphicsSceneEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneWheelEvent>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QGraphicsWidget>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHash>
#include <QHeaderView>
#include <QHostAddress>
#include <QHostInfo>
#include <QIcon>
#include <QImage>
#include <QImageReader>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLayoutItem>
#include <QLineEdit>
#include <QList>
#include <QListView>
#include <QListWidget>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QMetaEnum>
#include <QMetaObject>
#include <QMultiHash>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPair>
#include <QPalette>
#include <QParallelAnimationGroup>
#include <QPauseAnimation>
#include <QPen>
#include <QPixmap>
#include <QPixmapCache>
#include <QPoint>
#include <QPointer>
#include <QProcess>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRadioButton>
#include <QRandomGenerator>
#include <QRect>
#include <QRectF>
#include <QRegion>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <QScrollBar>
#include <QSemaphore>
#include <QSequentialAnimationGroup>
#include <QSet>
#include <QSettings>
#include <QSharedPointer>
#include <QShowEvent>
#include <QSignalMapper>
#include <QSize>
#include <QSpinBox>
#include <QSplitter>
#include <QStack>
#include <QStandardPaths>
#include <QStatusBar>
#include <QString>
#include <QStringList>
#include <QStyleFactory>
#include <QStyleOptionGraphicsItem>
#include <QSystemTrayIcon>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextItem>
#include <QTextOption>
#include <QTextStream>
#include <QThread>
#include <QThreadStorage>
#include <QTime>
#include <QTimer>
#include <QTimerEvent>
#include <QToolButton>
#include <QTransform>
#include <QTranslator>
#include <QUdpSocket>
#include <QUrl>
#include <QVBoxLayout>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QVersionNumber>
#include <QWaitCondition>
#include <QWidget>

#ifdef QT_MULTIMEDIA_LIB
#include <QAudioFormat>
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
#include <QAudioSink>
#else
#include <QAudioOutput>
#endif
#endif

#ifdef QT_WINEXTRAS_LIB
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#endif

#include <lua.hpp>
#endif

#ifdef AUDIO_SUPPORT
#include <vorbis/vorbisfile.h>
#endif

#endif
