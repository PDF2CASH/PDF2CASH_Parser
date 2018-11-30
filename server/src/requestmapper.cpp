/**
  @file
  @author Stefan Frings
*/

#include <QCoreApplication>
#include "requestmapper.h"
#include "filelogger.h"
#include "staticfilecontroller.h"
#include "controller/dumpcontroller.h"
#include "controller/templatecontroller.h"
#include "controller/formcontroller.h"
#include "controller/fileuploadcontroller.h"
#include "controller/sessioncontroller.h"
#include "controller/parsercontroller.h"

/** Redirects log messages to a file */
extern FileLogger* logger;

/** Controller for static files */
extern StaticFileController* staticFileController;

RequestMapper::RequestMapper(QSettings* settingsCors, QObject* parent)
    :HttpRequestHandler(parent)
{
    qDebug("RequestMapper: created");

    _settingCors = new CORS_SETTINGS();
    _settingCors->useOrigin = (settingsCors->value("useOrigin", "1").toInt() == 1) ? true : false;
    _settingCors->useMethods = (settingsCors->value("useMethods", "1").toInt() == 1) ? true : false;
    _settingCors->useHeaders = (settingsCors->value("useHeaders", "1").toInt() == 1) ? true : false;
    _settingCors->useMaxAge = (settingsCors->value("useMaxAge", "1").toInt() == 1) ? true : false;

    _settingCors->originData = settingsCors->value("origin", "*").toString();

    QString methodStr = settingsCors->value("methods", "POST, GET, OPTIONS, DELETE").toString();
    methodStr = methodStr.replace(":", ", ");

    _settingCors->methodsData = QString(methodStr);
    _settingCors->headersData = settingsCors->value("headers", "Content-Type").toString();
    _settingCors->maxAgeData = settingsCors->value("maxAge", "60000").toInt();
}


RequestMapper::~RequestMapper()
{
    qDebug("RequestMapper: deleted");
}


void RequestMapper::service(HttpRequest& request, HttpResponse& response)
{
    QByteArray path=request.getPath();
    qDebug("RequestMapper: path=%s",path.data());

    // For the following pathes, each request gets its own new instance of the related controller.

    if (path.startsWith("/dump"))
    {
        DumpController().service(request, response);
    }

    else if (path.startsWith("/template"))
    {
        TemplateController().service(request, response);
    }

    else if (path.startsWith("/form"))
    {
        FormController().service(request, response);
    }

    else if (path.startsWith("/file"))
    {
        FileUploadController().service(request, response);
    }

    else if (path.startsWith("/session"))
    {
        SessionController().service(request, response);
    }

    else if(path.startsWith("/parser"))
    {
        ParserController().service(request, response, _settingCors);
    }

    // All other pathes are mapped to the static file controller.
    // In this case, a single instance is used for multiple requests.
    else
    {
        staticFileController->service(request, response);
    }

    qDebug("RequestMapper: finished request");

    // Clear the log buffer
    if (logger)
    {
       logger->clear();
    }
}
