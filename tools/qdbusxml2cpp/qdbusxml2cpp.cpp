/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

// Upstream: https://gitee.com/GXDE-OS/gxde-qt-dbus-factory.git
// Originally modified by Deepin Technology
// Patched by CharofString to build against Qt6

#define QT6_ONLY(x) x

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QSet>
#include <QMap>

#include <QDBusMetaType>
#include <private/qdbusintrospection_p.h>

#include <stdio.h>
#include <stdlib.h>

#define PROGRAMNAME     "gxde-qdbusxml2cpp"
#define PROGRAMVERSION  "0.9"
#define PROGRAMCOPYRIGHT "Copyright (C) 2026 CharOfString"

#define ANNOTATION_NO_WAIT      "org.freedesktop.DBus.Method.NoReply"

static QString globalClassName;
static QString parentClassName;
static QString proxyFile;
static QString adaptorFile;
static QString inputFile;
static bool skipNamespaces;
static bool verbose;
static bool includeMocs;
static QString commandLine;
static QStringList includes;
static QStringList wantedInterfaces;

static const char help[] =
    "Usage: " PROGRAMNAME " [options...] [xml-or-xml-file] [interfaces...]\n"
    "Produces the C++ code to implement the interfaces defined in the input file.\n"
    "\n"
    "Options:\n"
    "  -a <filename>    Write the adaptor code to <filename>\n"
    "  -c <classname>   Use <classname> as the class name for the generated classes\n"
    "  -h               Show this information\n"
    "  -i <filename>    Add #include to the output\n"
    "  -l <classname>   When generating an adaptor, use <classname> as the parent class\n"
    "  -m               Generate #include \"filename.moc\" statements in the .cpp files\n"
    "  -N               Don't use namespaces\n"
    "  -p <filename>    Write the proxy code to <filename>\n"
    "  -v               Be verbose.\n"
    "  -V               Show the program version and quit.\n"
    "\n"
    "If the file name given to the options -a and -p does not end in .cpp or .h, the\n"
    "program will automatically append the suffixes and produce both files.\n"
    "You can also use a colon (:) to separate the header name from the source file\n"
    "name, as in '-a filename_p.h:filename.cpp'.\n"
    "\n"
    "If you pass a dash (-) as the argument to either -p or -a, the output is written\n"
    "to the standard output\n";

static const char includeList[] =
    "#include <QtCore/QByteArray>\n"
    "#include <QtCore/QList>\n"
    "#include <QtCore/QMap>\n"
    "#include <QtCore/QString>\n"
    "#include <QtCore/QStringList>\n"
    "#include <QtCore/QVariant>\n"
    "\n"
    "#include <DBusExtendedAbstractInterface>\n";

static const char forwardDeclarations[] =
    "QT_BEGIN_NAMESPACE\n"
    "class QByteArray;\n"
    "template<class T> class QList;\n"
    "template<class Key, class Value> class QMap;\n"
    "class QString;\n"
    "class QStringList;\n"
    "class QVariant;\n"
    "QT_END_NAMESPACE\n";

static void showHelp()
{
    printf("%s", help);
    exit(0);
}

static void showVersion()
{
    printf("%s version %s\n", PROGRAMNAME, PROGRAMVERSION);
    printf("D-Bus binding tool for Qt\n");
    exit(0);
}

static QString nextArg(QStringList &args, int i, char opt)
{
    QString arg = args.value(i);
    if (arg.isEmpty()) {
        printf("-%c needs at least one argument\n", opt);
        exit(1);
    }
    return args.takeAt(i);
}

static void parseCmdLine(QStringList args)
{
    args.takeFirst();

    commandLine = QLatin1String(PROGRAMNAME " ");
    commandLine += args.join(QLatin1Char(' '));

    int i = 0;
    while (i < args.count()) {

        if (!args.at(i).startsWith(QLatin1Char('-'))) {
            ++i;
            continue;
        }
        QString arg = args.takeAt(i);

        char c = '\0';
        if (arg.length() == 2)
            c = arg.at(1).toLatin1();
        else if (arg == QLatin1String("--help"))
            c = 'h';

        switch (c) {
        case 'a':
            adaptorFile = nextArg(args, i, 'a');
            break;

        case 'c':
            globalClassName = nextArg(args, i, 'c');
            break;

        case 'v':
            verbose = true;
            break;

        case 'i':
            includes << nextArg(args, i, 'i');
            break;

        case 'l':
            parentClassName = nextArg(args, i, 'l');
            break;

        case 'm':
            includeMocs = true;
            break;

        case 'N':
            skipNamespaces = true;
            break;

        case '?':
        case 'h':
            showHelp();
            break;

        case 'V':
            showVersion();
            break;

        case 'p':
            proxyFile = nextArg(args, i, 'p');
            break;

        default:
            printf("unknown option: '%s'\n", qPrintable(arg));
            exit(1);
        }
    }

    if (!args.isEmpty())
        inputFile = args.takeFirst();

    wantedInterfaces << args;
}

static QDBusIntrospection::Interfaces readInput()
{
    QFile input(inputFile);
    if (inputFile.isEmpty() || inputFile == QLatin1String("-")) {
        input.open(stdin, QIODevice::ReadOnly);
    } else {
        input.open(QIODevice::ReadOnly);
    }

    QByteArray data = input.readAll();

    // check if the input is already XML
    data = data.trimmed();
    if (data.startsWith("<!DOCTYPE ") || data.startsWith("<?xml") ||
        data.startsWith("<node") || data.startsWith("<interface"))
        // already XML
        return QDBusIntrospection::parseInterfaces(QString::fromUtf8(data));

    fprintf(stderr, "Cannot process input: '%s'. Stop.\n", qPrintable(inputFile));
    exit(1);
}

static void cleanInterfaces(QDBusIntrospection::Interfaces &interfaces)
{
    if (!wantedInterfaces.isEmpty()) {
        QDBusIntrospection::Interfaces::Iterator it = interfaces.begin();
        while (it != interfaces.end())
            if (!wantedInterfaces.contains(it.key()))
                it = interfaces.erase(it);
            else
                ++it;
    }
}

// produce a header name from the file name
static QString header(const QString &name)
{
    QStringList parts = name.split(QLatin1Char(':'));
    QString retval = parts.first();

    if (retval.isEmpty() || retval == QLatin1String("-"))
        return retval;

    if (!retval.endsWith(QLatin1String(".h")) && !retval.endsWith(QLatin1String(".cpp")) &&
        !retval.endsWith(QLatin1String(".cc")))
        retval.append(QLatin1String(".h"));

    return retval;
}

// produce a cpp name from the file name
static QString cpp(const QString &name)
{
    QStringList parts = name.split(QLatin1Char(':'));
    QString retval = parts.last();

    if (retval.isEmpty() || retval == QLatin1String("-"))
        return retval;

    if (!retval.endsWith(QLatin1String(".h")) && !retval.endsWith(QLatin1String(".cpp")) &&
        !retval.endsWith(QLatin1String(".cc")))
        retval.append(QLatin1String(".cpp"));

    return retval;
}

// produce a moc name from the file name
static QString moc(const QString &name)
{
    QString retval = header(name);
    if (retval.isEmpty())
        return retval;

    retval.truncate(retval.length() - 1); // drop the h in .h
    retval += QLatin1String("moc");
    return retval;
}

static QTextStream &writeHeader(QTextStream &ts, bool changesWillBeLost)
{
    ts << "/*" << Qt::endl
       << " * This file was generated by " PROGRAMNAME " version " PROGRAMVERSION << Qt::endl
       << " * Command line was: " << commandLine << Qt::endl
       << " *" << Qt::endl
       << " * " PROGRAMNAME " is " PROGRAMCOPYRIGHT << Qt::endl
       << " *" << Qt::endl
       << " * This is an auto-generated file." << Qt::endl;

    if (changesWillBeLost) {
        ts << " * Do not edit! All changes made to it will be lost." << Qt::endl;
    } else {
        ts << " * This file may have been hand-edited. Look for HAND-EDIT comments" << Qt::endl
           << " * before re-generating it." << Qt::endl;
    }

    ts << " */" << Qt::endl
       << Qt::endl;

    return ts;
}

enum ClassType { Proxy, Adaptor };
static QString classNameForInterface(const QString &interface, ClassType classType)
{
    if (!globalClassName.isEmpty())
        return globalClassName;

    QStringList parts = interface.split(QLatin1Char('.'));

    QString retval;
    if (classType == Proxy)
        foreach (QString part, parts) {
            part[0] = part[0].toUpper();
            retval += part;
        }
    else {
        retval = parts.last();
        retval[0] = retval[0].toUpper();
    }

    if (classType == Proxy)
        retval += QLatin1String("Interface");
    else
        retval += QLatin1String("Adaptor");

    return retval;
}

// Qt6 replacement for QDBusMetaType::signatureToType
// That one is removed in Qt6
static QString dbusSignatureToQtType(const QString &sig) {
    // Basic types
    if (sig == "s") return "QString";
    if (sig == "i") return "int";
    if (sig == "u") return "uint";
    if (sig == "b") return "bool";
    if (sig == "d") return "double";
    if (sig == "y") return "uchar";
    if (sig == "q") return "ushort";
    if (sig == "t") return "qlonglong";
    if (sig == "x") return "qulonglong";
    if (sig == "n") return "short";
    if (sig == "o") return "QDBusObjectPath";
    if (sig == "g") return "QDBusSignature";
    if (sig == "v") return "QVariant";
    if (sig == "as") return "QStringList";
    if (sig == "ay") return "QByteArray";
    if (sig == "a{sv}") return "QVariantMap";
    if (sig == "a{ss}") return "QMap<QString,QString>";
    // Complex/annotated types - just use the signature as-is
    return QString();
}

static QByteArray qtTypeName(const QString &signature, const QDBusIntrospection::Annotations &annotations, int paramId = -1, const char *direction = "Out")
{
    QString qtSig = dbusSignatureToQtType(signature);
    int type = qtSig.isEmpty() ? QMetaType::UnknownType : QMetaType::fromName(qtSig.toLatin1()).id();
    if (type == QVariant::Invalid) {
        QString annotationName = QString::fromLatin1("org.qtproject.QtDBus.QtTypeName");
        if (paramId >= 0)
            annotationName += QString::fromLatin1(".%1%2").arg(QLatin1String(direction)).arg(paramId);
        QString qttype = annotations.value(annotationName).value;
        if (!qttype.isEmpty())
            return qttype.toLatin1();

        QString oldAnnotationName = QString::fromLatin1("com.trolltech.QtDBus.QtTypeName");
        if (paramId >= 0)
            oldAnnotationName += QString::fromLatin1(".%1%2").arg(QLatin1String(direction)).arg(paramId);
        qttype = annotations.value(oldAnnotationName).value;

        if (qttype.isEmpty()) {
            fprintf(stderr, "Got unknown type `%s'\n", qPrintable(signature));
            fprintf(stderr, "You should add <annotation name=\"%s\" value=\"<type>\"/> to the XML description\n",
                    qPrintable(annotationName));
            return QByteArray();
        }

        fprintf(stderr, "Warning: deprecated annotation '%s' found; suggest updating to '%s'\n",
                qPrintable(oldAnnotationName), qPrintable(annotationName));
        return qttype.toLatin1();
    }

    return QVariant::typeToName(QVariant::Type(type));
}

static QString nonConstRefArg(const QByteArray &arg)
{
    return QLatin1String(arg + " &");
}

static QString templateArg(const QByteArray &arg)
{
    if (!arg.endsWith('>'))
        return QLatin1String(arg);

    return QLatin1String(arg + ' ');
}

static QString constRefArg(const QByteArray &arg)
{
    if (!arg.startsWith('Q'))
        return QLatin1String(arg + ' ');
    else
        return QString( QLatin1String("const %1 &") ).arg( QLatin1String(arg) );
}

static QStringList makeArgNames(const QDBusIntrospection::Arguments &inputArgs,
                                const QDBusIntrospection::Arguments &outputArgs =
                                QDBusIntrospection::Arguments())
{
    QStringList retval;
    const int numInputArgs = inputArgs.count();
    const int numOutputArgs = outputArgs.count();
    retval.reserve(numInputArgs + numOutputArgs);
    for (int i = 0; i < numInputArgs; ++i) {
        const QDBusIntrospection::Argument &arg = inputArgs.at(i);
        QString name = arg.name;
        if (name.isEmpty())
            name = QString( QLatin1String("in%1") ).arg(i);
        else
            name.replace(QLatin1Char('-'), QLatin1Char('_'));
        while (retval.contains(name))
            name += QLatin1String("_");
        retval << name;
    }
    for (int i = 0; i < numOutputArgs; ++i) {
        const QDBusIntrospection::Argument &arg = outputArgs.at(i);
        QString name = arg.name;
        if (name.isEmpty())
            name = QString( QLatin1String("out%1") ).arg(i);
        else
            name.replace(QLatin1Char('-'), QLatin1Char('_'));
        while (retval.contains(name))
            name += QLatin1String("_");
        retval << name;
    }
    return retval;
}

static void writeArgList(QTextStream &ts, const QStringList &argNames,
                         const QDBusIntrospection::Annotations &annotations,
                         const QDBusIntrospection::Arguments &inputArgs,
                         const QDBusIntrospection::Arguments &outputArgs = QDBusIntrospection::Arguments())
{
    // input args:
    bool first = true;
    int argPos = 0;
    for (int i = 0; i < inputArgs.count(); ++i) {
        const QDBusIntrospection::Argument &arg = inputArgs.at(i);
        QString type = constRefArg(qtTypeName(arg.type, annotations, i, "In"));

        if (!first)
            ts << ", ";
        ts << type << argNames.at(argPos++);
        first = false;
    }

    argPos++;

    // output args
    // yes, starting from 1
    for (int i = 1; i < outputArgs.count(); ++i) {
        const QDBusIntrospection::Argument &arg = outputArgs.at(i);
        QString name = arg.name;

        if (!first)
            ts << ", ";
        ts << nonConstRefArg(qtTypeName(arg.type, annotations, i, "Out"))
           << argNames.at(argPos++);
        first = false;
    }
}

static QString propertyGetter(const QDBusIntrospection::Property &property)
{
    QString getter = property.annotations.value(QLatin1String("org.qtproject.QtDBus.PropertyGetter")).value;
    if (!getter.isEmpty())
        return getter;

    getter = property.annotations.value(QLatin1String("com.trolltech.QtDBus.propertyGetter")).value;
    if (!getter.isEmpty()) {
        fprintf(stderr, "Warning: deprecated annotation 'com.trolltech.QtDBus.propertyGetter' found;"
                " suggest updating to 'org.qtproject.QtDBus.PropertyGetter'\n");
        return getter;
    }

    getter =  property.name;
    getter[0] = getter[0].toLower();
    return getter;
}

static QString propertySetter(const QDBusIntrospection::Property &property)
{
    QString setter = property.annotations.value(QLatin1String("org.qtproject.QtDBus.PropertySetter")).value;
    if (!setter.isEmpty())
        return setter;

    setter = property.annotations.value(QLatin1String("com.trolltech.QtDBus.propertySetter")).value;
    if (!setter.isEmpty()) {
        fprintf(stderr, "Warning: deprecated annotation 'com.trolltech.QtDBus.propertySetter' found;"
                " suggest updating to 'org.qtproject.QtDBus.PropertySetter'\n");
        return setter;
    }

    setter = QLatin1String("set") + property.name;
    setter[3] = setter[3].toUpper();
    return setter;
}

static QString propertyNotifier(const QDBusIntrospection::Property &property)
{
    QString notifier = property.annotations.value(QLatin1String("org.qtproject.QtDBus.PropertyNotifier")).value;
    if (!notifier.isEmpty())
        return notifier;

    notifier=  property.name + QLatin1String("Changed");
    notifier[0] = notifier[0].toUpper();
    return notifier;
}

static QString methodName(const QDBusIntrospection::Method &method)
{
    QString name = method.annotations.value(QStringLiteral("org.qtproject.QtDBus.MethodName")).value;
    if (!name.isEmpty())
        return name;

    return method.name;
}

static QString stringify(const QString &data)
{
    QString retval;
    int i;
    for (i = 0; i < data.length(); ++i) {
        retval += QLatin1Char('\"');
        for ( ; i < data.length() && data[i] != QLatin1Char('\n') && data[i] != QLatin1Char('\r'); ++i)
            if (data[i] == QLatin1Char('\"'))
                retval += QLatin1String("\\\"");
            else
                retval += data[i];
        if (i+1 < data.length() && data[i] == QLatin1Char('\r') && data[i+1] == QLatin1Char('\n'))
            i++;
        retval += QLatin1String("\\n\"\n");
    }
    return retval;
}

static bool openFile(const QString &fileName, QFile &file)
{
    if (fileName.isEmpty())
        return false;

    bool isOk = false;
    if (fileName == QLatin1String("-")) {
        isOk = file.open(stdout, QIODevice::WriteOnly | QIODevice::Text);
    } else {
        file.setFileName(fileName);
        isOk = file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    }

    if (!isOk)
        fprintf(stderr, "Unable to open '%s': %s\n", qPrintable(fileName),
                qPrintable(file.errorString()));
    return isOk;
}

static void writeProxy(const QString &filename, const QDBusIntrospection::Interfaces &interfaces)
{
    // open the file
    QString headerName = header(filename);
    QByteArray headerData;
    QTextStream hs(&headerData);

    QString cppName = cpp(filename);
    QByteArray cppData;
    QTextStream cs(&cppData);

    // write the header:
    writeHeader(hs, true);
    if (cppName != headerName)
        writeHeader(cs, false);

    // include guards:
    QString includeGuard;
    if (!headerName.isEmpty() && headerName != QLatin1String("-")) {
        includeGuard = headerName.toUpper().replace(QLatin1Char('.'), QLatin1Char('_'));
        int pos = includeGuard.lastIndexOf(QLatin1Char('/'));
        if (pos != -1)
            includeGuard = includeGuard.mid(pos + 1);
    } else {
        includeGuard = QLatin1String("QDBUSXML2CPP_PROXY");
    }
    includeGuard = QString(QLatin1String("%1"))
                   .arg(includeGuard);
    hs << "#ifndef " << includeGuard << Qt::endl
       << "#define " << includeGuard << Qt::endl
       << Qt::endl;

    // include our stuff:
    hs << "#include <QtCore/QObject>" << Qt::endl
       << includeList
       << "#include <QtDBus/QtDBus>" << Qt::endl;

    foreach (const QString &include, includes) {
        hs << "#include \"" << include << "\"" << Qt::endl;
        if (headerName.isEmpty())
            cs << "#include \"" << include << "\"" << Qt::endl;
    }

    hs << Qt::endl;

    if (cppName != headerName) {
        if (!headerName.isEmpty() && headerName != QLatin1String("-"))
            cs << "#include \"" << headerName << "\"" << Qt::endl << Qt::endl;
    }

    QSet<QString> annotations;
    for (const QDBusIntrospection::Interface *interface : interfaces)
    {
        for (const auto method : interface->methods)
        {
            for (int i(0); i != method.outputArgs.size(); ++i)
            {
                const QDBusIntrospection::Argument &arg = method.outputArgs[i];
                if (QMetaType::fromName(dbusSignatureToQtType(arg.type).toLatin1()).id() != QVariant::Invalid)
                    continue;

                annotations << qtTypeName(arg.type, method.annotations, i, "Out");
            }

            for (int i(0); i != method.inputArgs.size(); ++i)
            {
                const QDBusIntrospection::Argument &arg = method.inputArgs[i];
                if (QMetaType::fromName(dbusSignatureToQtType(arg.type).toLatin1()).id() != QVariant::Invalid)
                    continue;

                annotations << qtTypeName(arg.type, method.annotations, i, "In");
            }
        }

        for (const auto property : interface->properties)
        {
            if (QMetaType::fromName(dbusSignatureToQtType(property.type).toLatin1()).id() != QVariant::Invalid)
                continue;

            annotations << qtTypeName(property.type, property.annotations);
        }
    }

    for (const QString &annotation : annotations) {
        if (annotation.indexOf('<')==-1) {
            hs << "#include \"types/" << annotation.toLower() << ".h\"" << Qt::endl;
        }
    }
    hs << Qt::endl;

    foreach (const QDBusIntrospection::Interface *interface, interfaces) {
        QString className = "__" + classNameForInterface(interface->name, Proxy);

        // comment:
        hs << "/*" << Qt::endl
           << " * Proxy class for interface " << interface->name << Qt::endl
           << " */" << Qt::endl;
        cs << "/*" << Qt::endl
           << " * Implementation of interface class " << className << Qt::endl
           << " */" << Qt::endl
           << Qt::endl;

        // private class declare
        hs << "class " << className << "Private;" << Qt::endl;
        // class header:
        hs << "class " << className << " : public DBusExtendedAbstractInterface" << Qt::endl
           << "{" << Qt::endl
           << "    Q_OBJECT" << Qt::endl;
        hs << Qt::endl;

        // private class defines
        cs << "class " << className << "Private" << Qt::endl
           << "{" << Qt::endl
           << "public:" << Qt::endl
           << "   " << className << "Private() = default;" << Qt::endl
           << Qt::endl;

        // private class member
        cs << "    // begin member variables" << Qt::endl;
        for (const auto property : interface->properties)
        {
            QByteArray type = qtTypeName(property.type, property.annotations);
            cs << "    " << type << " " << property.name << ';' << Qt::endl;
        }

        cs << Qt::endl;

        // stuffs member
        cs << "public:" << Qt::endl
           << "    QMap<QString, QDBusPendingCallWatcher *> m_processingCalls;" << Qt::endl
           << "    QMap<QString, QList<QVariant>> m_waittingCalls;" << Qt::endl;

        cs << "};" << Qt::endl
           << Qt::endl;
        // end of private class defines

        // the interface name
        hs << "public:" << Qt::endl
           << "    static inline const char *staticInterfaceName()" << Qt::endl
           << "    { return \"" << interface->name << "\"; }" << Qt::endl
           << Qt::endl;

        // constructors/destructors:
        hs << "public:" << Qt::endl
           << "    explicit " << className << "(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);" << Qt::endl
           << Qt::endl
           << "    ~" << className << "();" << Qt::endl
           << Qt::endl;
        cs << className << "::" << className << "(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent)" << Qt::endl
           << "    : DBusExtendedAbstractInterface(service, path, staticInterfaceName(), connection, parent)" << Qt::endl
           << "    , d_ptr(new " << className << "Private)" << Qt::endl
           << "{" << Qt::endl;
        if (!interface->properties.isEmpty())
            cs   << "    connect(this, &" << className << "::propertyChanged, this, &" << className << "::onPropertyChanged);" << Qt::endl << Qt::endl;

        for (const QString &annotation : annotations) {
            if(annotation.indexOf('<')!=-1) {
                cs << "    if (QMetaType::type(\"" << annotation << "\") == QMetaType::UnknownType) {" << Qt::endl;
                cs << "        qRegisterMetaType< " << annotation << " >(\"" << annotation << "\");" << Qt::endl;
                cs << "        qDBusRegisterMetaType< " << annotation << " >();" << Qt::endl;
                cs << "    }" << Qt::endl;
            }
            else {
                cs << "    if (QMetaType::type(\"" << annotation << "\") == QMetaType::UnknownType)" << Qt::endl;
                cs << "        register" << annotation << "MetaType();" << Qt::endl;
            }
        }

        cs << "}" << Qt::endl
           << Qt::endl
           << className << "::~" << className << "()" << Qt::endl
           << "{" << Qt::endl
           << "    qDeleteAll(d_ptr->m_processingCalls.values());" << Qt::endl
           << "    delete d_ptr;" << Qt::endl
           << "}" << Qt::endl
           << Qt::endl;

        if (!interface->properties.isEmpty())
        {
            // onPropertyChanged
            cs << "void " << className << "::onPropertyChanged(const QString &propName, const QVariant &value)" << Qt::endl;
            cs << "{" << Qt::endl;

            for (const auto property : interface->properties)
            {
                char first = property.name[0].toLatin1();
                QString name = property.name;
                name[0] = QChar(first).toUpper();

                QByteArray type = qtTypeName(property.type, property.annotations);

                cs << "    if (propName == QStringLiteral(\"" << property.name << "\"))" << Qt::endl;
                cs << "    {" << Qt::endl;
                cs << "        const " << type << " &" << property.name << " = qvariant_cast<" << type << ">(value);" << Qt::endl;
                cs << "        " << "if (d_ptr->" << property.name << " != " << property.name << ")" << Qt::endl;
                cs << "        {" << Qt::endl;
                cs << "            d_ptr->" << property.name << " = " << property.name << ';' << Qt::endl;
                cs << "            Q_EMIT " << name << "Changed(d_ptr->" << property.name << ");" << Qt::endl;
                cs << "        }" << Qt::endl;
                cs << "        return;" << Qt::endl;
                cs << "    }" << Qt::endl;
                cs << Qt::endl;
            }
            cs << "    qWarning() << \"property not handle: \" << propName;" << Qt::endl;
            cs << "    return;" << Qt::endl;
            cs << "}" << Qt::endl
               << Qt::endl;
        }

        // properties:
        foreach (const QDBusIntrospection::Property &property, interface->properties) {
            QByteArray type = qtTypeName(property.type, property.annotations);
//            QString templateType = templateArg(type);
//            QString constRefType = constRefArg(type);
            QString getter = propertyGetter(property);
            QString setter = propertySetter(property);
            QString notifier = propertyNotifier(property);

            hs << "    Q_PROPERTY(" << type << " " << property.name;

            // getter:
            if (property.access != QDBusIntrospection::Property::Write)
                // it's readble
                hs << " READ " << getter;

            // setter
            if (property.access != QDBusIntrospection::Property::Read)
                // it's writeable
                hs << " WRITE " << setter;

            //notifier
            hs << " NOTIFY " << notifier;

            hs << ")" << Qt::endl;

            // getter:
            if (property.access != QDBusIntrospection::Property::Write) {
                // getter declare
                hs << "    " << type << " " << getter << "();" << Qt::endl;

                // getter define
                cs << type << " " << className << "::" << getter << "()" << Qt::endl
                   << "{" << Qt::endl
                   << "    return qvariant_cast<" << type << ">(internalPropGet(\"" << property.name << "\", &d_ptr->" << property.name << "));" << Qt::endl
                   << "}" << Qt::endl
                   << Qt::endl;
            }

            // setter:
            if (property.access != QDBusIntrospection::Property::Read) {
                // setter declare
                hs << "    void " << setter << "(" << constRefArg(type) << "value);" << Qt::endl;

                // setter define
                cs << "void " << className << "::" << setter << "(" << constRefArg(type) << "value)" << Qt::endl
                   << "{" << Qt::endl
                   << Qt::endl
                   << "   internalPropSet(\"" << property.name << "\", QVariant::fromValue(value), &d_ptr->" << property.name << ");" << Qt::endl
                   << "}" << Qt::endl
                   << Qt::endl;
            }

            hs << Qt::endl;
        }

        // methods:
        hs << "public Q_SLOTS: // METHODS" << Qt::endl;
        foreach (const QDBusIntrospection::Method &method, interface->methods) {
            bool isDeprecated = method.annotations.value(QLatin1String("org.freedesktop.DBus.Deprecated")).value == QLatin1String("true");
            bool isNoReply =
                method.annotations.value(QLatin1String(ANNOTATION_NO_WAIT)).value == QLatin1String("true");
            if (isNoReply && !method.outputArgs.isEmpty()) {
                fprintf(stderr, "warning: method %s in interface %s is marked 'no-reply' but has output arguments.\n",
                        qPrintable(method.name), qPrintable(interface->name));
                continue;
            }

            hs << "    inline "
               << (isDeprecated ? "Q_DECL_DEPRECATED " : "");

            if (isNoReply) {
                hs << "Q_NOREPLY void ";
            } else {
                hs << "QDBusPendingReply<";
                for (int i = 0; i < method.outputArgs.count(); ++i)
                    hs << (i > 0 ? ", " : "")
                       << templateArg(qtTypeName(method.outputArgs.at(i).type, method.annotations, i, "Out"));
                hs << "> ";
            }

            hs << methodName(method) << "(";

            QStringList argNames = makeArgNames(method.inputArgs);
            writeArgList(hs, argNames, method.annotations, method.inputArgs);

            hs << ")" << Qt::endl
               << "    {" << Qt::endl
               << "        QList<QVariant> argumentList;" << Qt::endl;

            if (!method.inputArgs.isEmpty()) {
                hs << "        argumentList";
                for (int argPos = 0; argPos < method.inputArgs.count(); ++argPos)
                    hs << " << QVariant::fromValue(" << argNames.at(argPos) << ')';
                hs << ";" << Qt::endl;
            }

            if (isNoReply)
                hs << "        callWithArgumentList(QDBus::NoBlock, "
                   <<  "QStringLiteral(\"" << method.name << "\"), argumentList);" << Qt::endl;
            else
                hs << "        return asyncCallWithArgumentList(QStringLiteral(\""
                   << method.name << "\"), argumentList);" << Qt::endl;

            // close the function:
            hs << "    }" << Qt::endl;

            hs << Qt::endl;
            // queued version for void return type functions
            if (method.outputArgs.count() == 0)
            {
                hs << "    inline void " << method.name << "Queued(";
                writeArgList(hs, argNames, method.annotations, method.inputArgs, method.outputArgs);
                hs << ")" << Qt::endl
                   << "    {" << Qt::endl
                   << "        QList<QVariant> argumentList;" << Qt::endl;

                int argPos = 0;
                if (!method.inputArgs.isEmpty()) {
                    hs << "        argumentList";
                    for (argPos = 0; argPos < method.inputArgs.count(); ++argPos)
                        hs << " << QVariant::fromValue(" << argNames.at(argPos) << ')';
                    hs << ";" << Qt::endl;
                }

                hs << Qt::endl
                   << "        CallQueued(" << "QStringLiteral(\"" << method.name << "\"), argumentList);" << Qt::endl
                   << "    }" << Qt::endl;
            }

            hs << Qt::endl;
            if (method.outputArgs.count() > 1) {
                const auto templateArgument = templateArg(qtTypeName(method.outputArgs.first().type, method.annotations, 0, "Out"));

                // generate the old-form QDBusReply methods with multiple incoming parameters
                hs << "    inline "
                   << (isDeprecated ? "Q_DECL_DEPRECATED " : "")
                   << "QDBusReply<"
                   << templateArgument << "> ";
                hs << method.name << "(";

                QStringList argNames = makeArgNames(method.inputArgs, method.outputArgs);
                writeArgList(hs, argNames, method.annotations, method.inputArgs, method.outputArgs);

                hs << ")" << Qt::endl
                   << "    {" << Qt::endl
                   << "        QList<QVariant> argumentList;" << Qt::endl;

                int argPos = 0;
                if (!method.inputArgs.isEmpty()) {
                    hs << "        argumentList";
                    for (argPos = 0; argPos < method.inputArgs.count(); ++argPos)
                        hs << " << QVariant::fromValue(" << argNames.at(argPos) << ')';
                    hs << ";" << Qt::endl;
                }

                hs << "        QDBusMessage reply = callWithArgumentList(QDBus::Block, "
                   <<  "QStringLiteral(\"" << method.name << "\"), argumentList);" << Qt::endl;

                argPos++;
                hs << "        if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().count() == "
                   << method.outputArgs.count() << ") {" << Qt::endl;

                // yes, starting from 1
                for (int i = 1; i < method.outputArgs.count(); ++i)
                    hs << "            " << argNames.at(argPos++) << " = qdbus_cast<"
                       << templateArg(qtTypeName(method.outputArgs.at(i).type, method.annotations, i, "Out"))
                       << ">(reply.arguments().at(" << i << "));" << Qt::endl;
                hs << "        }" << Qt::endl
                   << "        return reply;" << Qt::endl
                   << "    }" << Qt::endl;
            }

            hs << Qt::endl;
        }

        hs << Qt::endl;

        hs << "Q_SIGNALS: // SIGNALS" << Qt::endl;
        foreach (const QDBusIntrospection::Signal &signal, interface->signals_) {
            hs << "    ";
            if (signal.annotations.value(QLatin1String("org.freedesktop.DBus.Deprecated")).value ==
                QLatin1String("true"))
                hs << "Q_DECL_DEPRECATED ";

            hs << "void " << signal.name << "(";

            QStringList argNames = makeArgNames(signal.outputArgs);
            writeArgList(hs, argNames, signal.annotations, signal.outputArgs);

            hs << ");" << Qt::endl; // finished for header
        }

        //propery changed signals
        hs << "    // begin property changed signals" << Qt::endl;
        foreach (const QDBusIntrospection::Property &property, interface->properties) {
            hs << "    ";
            QByteArray type = qtTypeName(property.type, property.annotations);
            QString constRefType = constRefArg(type);
            QString notifier = propertyNotifier(property);

            //notifier
            hs << "void " << notifier << "(" << constRefType << " value" << ") const;" << Qt::endl;
        }

        hs << Qt::endl;

        // queued stuffs
        hs << "public Q_SLOTS:" << Qt::endl
           << "    void CallQueued(const QString &callName, const QList<QVariant> &args);" << Qt::endl
           << Qt::endl;

        cs << "void " << className << "::CallQueued(const QString &callName, const QList<QVariant> &args)" << Qt::endl
           << "{" << Qt::endl
           << "    if (d_ptr->m_waittingCalls.contains(callName))" << Qt::endl
           << "    {" << Qt::endl
           << "        d_ptr->m_waittingCalls[callName] = args;" << Qt::endl
           << "        return;" << Qt::endl
           << "    }" << Qt::endl
           << "    if (d_ptr->m_processingCalls.contains(callName))" << Qt::endl
           << "    {" << Qt::endl
           << "        d_ptr->m_waittingCalls.insert(callName, args);" << Qt::endl
           << "    } else {" << Qt::endl
           << "        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(asyncCallWithArgumentList(callName, args));" << Qt::endl
           << "        connect(watcher, &QDBusPendingCallWatcher::finished, this, &" << className << "::onPendingCallFinished);" << Qt::endl
           << "        d_ptr->m_processingCalls.insert(callName, watcher);" << Qt::endl
           << "    }" << Qt::endl
           << "}" << Qt::endl
           << Qt::endl;

        hs << "private Q_SLOTS:" << Qt::endl
           << "    void onPendingCallFinished(QDBusPendingCallWatcher *w);" << Qt::endl;

        if (!interface->properties.isEmpty())
            hs << "    void onPropertyChanged(const QString &propName, const QVariant &value);" << Qt::endl;

        hs << Qt::endl;

        cs << "void " << className << "::onPendingCallFinished(QDBusPendingCallWatcher *w)" << Qt::endl
           << "{" << Qt::endl
           << "    w->deleteLater();" << Qt::endl

           << "    const auto callName = d_ptr->m_processingCalls.key(w);" << Qt::endl
           << "    Q_ASSERT(!callName.isEmpty());" << Qt::endl
           << "    if (callName.isEmpty())" << Qt::endl
           << "        return;" << Qt::endl

           << "    d_ptr->m_processingCalls.remove(callName);" << Qt::endl

           << "    if (!d_ptr->m_waittingCalls.contains(callName))" << Qt::endl
           << "        return;" << Qt::endl

           << "    const auto args = d_ptr->m_waittingCalls.take(callName);" << Qt::endl
           << "    CallQueued(callName, args);" << Qt::endl
           << "}" << Qt::endl;

        // private member
        hs << "private:" << Qt::endl
           << "    " << className << "Private *d_ptr;" << Qt::endl;

        // close the class:
        hs << "};" << Qt::endl
           << Qt::endl;
    }

    if (!skipNamespaces) {
        QStringList last;
        QDBusIntrospection::Interfaces::ConstIterator it = interfaces.constBegin();
        do
        {
            QStringList current;
            QString name;
            if (it != interfaces.constEnd()) {
                current = it->constData()->name.split(QLatin1Char('.'));
                name = current.takeLast();
            }

            int i = 0;
            while (i < current.count() && i < last.count() && current.at(i) == last.at(i))
                ++i;

            // i parts matched
            // close last.arguments().count() - i namespaces:
            for (int j = i; j < last.count(); ++j)
                hs << QString((last.count() - j - 1 + i) * 2, QLatin1Char(' ')) << "}" << Qt::endl;

            // open current.arguments().count() - i namespaces
            for (int j = i; j < current.count(); ++j)
                hs << QString(j * 2, QLatin1Char(' ')) << "namespace " << current.at(j).toLower() << " {" << Qt::endl;

            // add this class:
            if (!name.isEmpty()) {
                hs << QString(current.count() * 2, QLatin1Char(' '))
                   << "typedef ::__" << classNameForInterface(it->constData()->name, Proxy)
                   << " " << name << ";" << Qt::endl;
            }

            if (it == interfaces.constEnd())
                break;
            ++it;
            last = current;
        } while (true);
    }

    // close the include guard
    hs << "#endif" << Qt::endl;

    QString mocName = moc(filename);
    if (includeMocs && !mocName.isEmpty())
        cs << Qt::endl
           << "#include \"" << mocName << "\"" << Qt::endl;

    cs.flush();
    hs.flush();

    QFile file;
    const bool headerOpen = openFile(headerName, file);
    if (headerOpen)
        file.write(headerData);

    if (headerName == cppName) {
        if (headerOpen)
            file.write(cppData);
    } else {
        QFile cppFile;
        if (openFile(cppName, cppFile))
            cppFile.write(cppData);
    }
}

static void writeAdaptor(const QString &filename, const QDBusIntrospection::Interfaces &interfaces)
{
    // open the file
    QString headerName = header(filename);
    QByteArray headerData;
    QTextStream hs(&headerData);

    QString cppName = cpp(filename);
    QByteArray cppData;
    QTextStream cs(&cppData);

    // write the headers
    writeHeader(hs, false);
    if (cppName != headerName)
        writeHeader(cs, true);

    // include guards:
    QString includeGuard;
    if (!headerName.isEmpty() && headerName != QLatin1String("-")) {
        includeGuard = headerName.toUpper().replace(QLatin1Char('.'), QLatin1Char('_'));
        int pos = includeGuard.lastIndexOf(QLatin1Char('/'));
        if (pos != -1)
            includeGuard = includeGuard.mid(pos + 1);
    } else {
        includeGuard = QLatin1String("QDBUSXML2CPP_ADAPTOR");
    }
    includeGuard = QString(QLatin1String("%1"))
                   .arg(includeGuard);
    hs << "#ifndef " << includeGuard << Qt::endl
       << "#define " << includeGuard << Qt::endl
       << Qt::endl;

    // include our stuff:
    hs << "#include <QtCore/QObject>" << Qt::endl;
    if (cppName == headerName)
        hs << "#include <QtCore/QMetaObject>" << Qt::endl
           << "#include <QtCore/QVariant>" << Qt::endl;
    hs << "#include <QtDBus/QtDBus>" << Qt::endl;

    foreach (const QString &include, includes) {
        hs << "#include \"" << include << "\"" << Qt::endl;
        if (headerName.isEmpty())
            cs << "#include \"" << include << "\"" << Qt::endl;
    }

    if (cppName != headerName) {
        if (!headerName.isEmpty() && headerName != QLatin1String("-"))
            cs << "#include \"" << headerName << "\"" << Qt::endl;

        cs << "#include <QtCore/QMetaObject>" << Qt::endl
           << includeList
           << Qt::endl;
        hs << forwardDeclarations;
    } else {
        hs << includeList;
    }

    hs << Qt::endl;

    QString parent = parentClassName;
    if (parentClassName.isEmpty())
        parent = QLatin1String("QObject");

    foreach (const QDBusIntrospection::Interface *interface, interfaces) {
        QString className = classNameForInterface(interface->name, Adaptor);

        // comment:
        hs << "/*" << Qt::endl
           << " * Adaptor class for interface " << interface->name << Qt::endl
           << " */" << Qt::endl;
        cs << "/*" << Qt::endl
           << " * Implementation of adaptor class " << className << Qt::endl
           << " */" << Qt::endl
           << Qt::endl;

        // class header:
        hs << "class " << className << ": public QDBusAbstractAdaptor" << Qt::endl
           << "{" << Qt::endl
           << "    Q_OBJECT" << Qt::endl
           << "    Q_CLASSINFO(\"D-Bus Interface\", \"" << interface->name << "\")" << Qt::endl
           << "    Q_CLASSINFO(\"D-Bus Introspection\", \"\"" << Qt::endl
           << stringify(interface->introspection)
           << "        \"\")" << Qt::endl
           << "public:" << Qt::endl
           << "    " << className << "(" << parent << " *parent);" << Qt::endl
           << "    virtual ~" << className << "();" << Qt::endl
           << Qt::endl;

        if (!parentClassName.isEmpty())
            hs << "    inline " << parent << " *parent() const" << Qt::endl
               << "    { return static_cast<" << parent << " *>(QObject::parent()); }" << Qt::endl
               << Qt::endl;

        // constructor/destructor
        cs << className << "::" << className << "(" << parent << " *parent)" << Qt::endl
           << "    : QDBusAbstractAdaptor(parent)" << Qt::endl
           << "{" << Qt::endl
           << "    // constructor" << Qt::endl
           << "    setAutoRelaySignals(true);" << Qt::endl
           << "}" << Qt::endl
           << Qt::endl
           << className << "::~" << className << "()" << Qt::endl
           << "{" << Qt::endl
           << "    // destructor" << Qt::endl
           << "}" << Qt::endl
           << Qt::endl;

        hs << "public: // PROPERTIES" << Qt::endl;
        foreach (const QDBusIntrospection::Property &property, interface->properties) {
            QByteArray type = qtTypeName(property.type, property.annotations);
            QString constRefType = constRefArg(type);
            QString getter = propertyGetter(property);
            QString setter = propertySetter(property);

            hs << "    Q_PROPERTY(" << type << " " << property.name;
            if (property.access != QDBusIntrospection::Property::Write)
                hs << " READ " << getter;
            if (property.access != QDBusIntrospection::Property::Read)
                hs << " WRITE " << setter;
            hs << ")" << Qt::endl;

            // getter:
            if (property.access != QDBusIntrospection::Property::Write) {
                hs << "    " << type << " " << getter << "() const;" << Qt::endl;
                cs << type << " "
                   << className << "::" << getter << "() const" << Qt::endl
                   << "{" << Qt::endl
                   << "    // get the value of property " << property.name << Qt::endl
                   << "    return qvariant_cast< " << type <<" >(parent()->property(\"" << property.name << "\"));" << Qt::endl
                   << "}" << Qt::endl
                   << Qt::endl;
            }

            // setter
            if (property.access != QDBusIntrospection::Property::Read) {
                hs << "    void " << setter << "(" << type << "value);" << Qt::endl;
                cs << "void " << className << "::" << setter << "(" << type << "value)" << Qt::endl
                   << "{" << Qt::endl
                   << "    // set the value of property " << property.name << Qt::endl
                   << "    parent()->setProperty(\"" << property.name << "\", QVariant::fromValue(value";
                if (constRefType.contains(QLatin1String("QDBusVariant")))
                    cs << ".variant()";
                cs << "));" << Qt::endl
                   << "}" << Qt::endl
                   << Qt::endl;
            }

            hs << Qt::endl;
        }

        hs << "public Q_SLOTS: // METHODS" << Qt::endl;
        foreach (const QDBusIntrospection::Method &method, interface->methods) {
            bool isNoReply =
                method.annotations.value(QLatin1String(ANNOTATION_NO_WAIT)).value == QLatin1String("true");
            if (isNoReply && !method.outputArgs.isEmpty()) {
                fprintf(stderr, "warning: method %s in interface %s is marked 'no-reply' but has output arguments.\n",
                        qPrintable(method.name), qPrintable(interface->name));
                continue;
            }

            hs << "    ";
            if (method.annotations.value(QLatin1String("org.freedesktop.DBus.Deprecated")).value ==
                QLatin1String("true"))
                hs << "Q_DECL_DEPRECATED ";

            QByteArray returnType;
            if (isNoReply) {
                hs << "Q_NOREPLY void ";
                cs << "void ";
            } else if (method.outputArgs.isEmpty()) {
                hs << "void ";
                cs << "void ";
            } else {
                returnType = qtTypeName(method.outputArgs.first().type, method.annotations, 0, "Out");
                hs << returnType << " ";
                cs << returnType << " ";
            }

            QString name = methodName(method);
            hs << name << "(";
            cs << className << "::" << name << "(";

            QStringList argNames = makeArgNames(method.inputArgs, method.outputArgs);
            writeArgList(hs, argNames, method.annotations, method.inputArgs, method.outputArgs);
            writeArgList(cs, argNames, method.annotations, method.inputArgs, method.outputArgs);

            hs << ");" << Qt::endl; // finished for header
            cs << ")" << Qt::endl
               << "{" << Qt::endl
               << "    // handle method call " << interface->name << "." << methodName(method) << Qt::endl;

            // make the call
            bool usingInvokeMethod = false;
            if (parentClassName.isEmpty() && method.inputArgs.count() <= 10
                && method.outputArgs.count() <= 1)
                usingInvokeMethod = true;

            if (usingInvokeMethod) {
                // we are using QMetaObject::invokeMethod
                if (!returnType.isEmpty())
                    cs << "    " << returnType << " " << argNames.at(method.inputArgs.count())
                       << ";" << Qt::endl;

                static const char invoke[] = "    QMetaObject::invokeMethod(parent(), \"";
                cs << invoke << name << "\"";

                if (!method.outputArgs.isEmpty())
                    cs << ", Q_RETURN_ARG("
                       << qtTypeName(method.outputArgs.at(0).type, method.annotations,
                                     0, "Out")
                       << ", "
                       << argNames.at(method.inputArgs.count())
                       << ")";

                for (int i = 0; i < method.inputArgs.count(); ++i)
                    cs << ", Q_ARG("
                       << qtTypeName(method.inputArgs.at(i).type, method.annotations,
                                     i, "In")
                       << ", "
                       << argNames.at(i)
                       << ")";

                cs << ");" << Qt::endl;

                if (!returnType.isEmpty())
                    cs << "    return " << argNames.at(method.inputArgs.count()) << ";" << Qt::endl;
            } else {
                if (parentClassName.isEmpty())
                    cs << "    //";
                else
                    cs << "    ";

                if (!method.outputArgs.isEmpty())
                    cs << "return ";

                if (parentClassName.isEmpty())
                    cs << "static_cast<YourObjectType *>(parent())->";
                else
                    cs << "parent()->";
                cs << name << "(";

                int argPos = 0;
                bool first = true;
                for (int i = 0; i < method.inputArgs.count(); ++i) {
                    cs << (first ? "" : ", ") << argNames.at(argPos++);
                    first = false;
                }
                ++argPos;           // skip retval, if any
                for (int i = 1; i < method.outputArgs.count(); ++i) {
                    cs << (first ? "" : ", ") << argNames.at(argPos++);
                    first = false;
                }

                cs << ");" << Qt::endl;
            }
            cs << "}" << Qt::endl
               << Qt::endl;
        }

        hs << "Q_SIGNALS: // SIGNALS" << Qt::endl;
        foreach (const QDBusIntrospection::Signal &signal, interface->signals_) {
            hs << "    ";
            if (signal.annotations.value(QLatin1String("org.freedesktop.DBus.Deprecated")).value ==
                QLatin1String("true"))
                hs << "Q_DECL_DEPRECATED ";

            hs << "void " << signal.name << "(";

            QStringList argNames = makeArgNames(signal.outputArgs);
            writeArgList(hs, argNames, signal.annotations, signal.outputArgs);

            hs << ");" << Qt::endl; // finished for header
        }

        // close the class:
        hs << "};" << Qt::endl
           << Qt::endl;
    }

    // close the include guard
    hs << "#endif" << Qt::endl;

    QString mocName = moc(filename);
    if (includeMocs && !mocName.isEmpty())
        cs << Qt::endl
           << "#include \"" << mocName << "\"" << Qt::endl;

    cs.flush();
    hs.flush();

    QFile file;
    const bool headerOpen = openFile(headerName, file);
    if (headerOpen)
        file.write(headerData);

    if (headerName == cppName) {
        if (headerOpen)
            file.write(cppData);
    } else {
        QFile cppFile;
        if (openFile(cppName, cppFile))
            cppFile.write(cppData);
    }
}

int main(int argc, char **argv)
{
    QStringList arguments;
    arguments.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        arguments.append(QString::fromLocal8Bit(argv[i]));
    }

    parseCmdLine(arguments);

    QDBusIntrospection::Interfaces interfaces = readInput();
    cleanInterfaces(interfaces);

    if (!proxyFile.isEmpty() || adaptorFile.isEmpty())
        writeProxy(proxyFile, interfaces);

    if (!adaptorFile.isEmpty())
        writeAdaptor(adaptorFile, interfaces);

    return 0;
}

