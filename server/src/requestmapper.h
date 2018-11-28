/**
  @file
  @author Stefan Frings
*/

#ifndef REQUESTMAPPER_H
#define REQUESTMAPPER_H

#include "httprequesthandler.h"

#include "./parser/Parser.h"

using namespace stefanfrings;

struct CORS_SETTINGS
{
    bool useOrigin;
    bool useMethods;
    bool useHeaders;
    bool useMaxAge;

    QString originData;
    QString methodsData;
    QString headersData;
    int maxAgeData;
};

/**
  The request mapper dispatches incoming HTTP requests to controller classes
  depending on the requested path.
*/

class RequestMapper : public HttpRequestHandler {
    Q_OBJECT
    Q_DISABLE_COPY(RequestMapper)
public:

    /**
      Constructor.
      @param parent Parent object
    */
    RequestMapper(QSettings* settingsCors, QObject* parent=0);

    /**
      Destructor.
    */
    ~RequestMapper();

    /**
      Dispatch incoming HTTP requests to different controllers depending on the URL.
      @param request The received HTTP request
      @param response Must be used to return the response
    */
    void service(HttpRequest& request, HttpResponse& response);

private:
    CORS_SETTINGS* _settingCors;
};

#endif // REQUESTMAPPER_H
