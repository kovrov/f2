#include "application.h"
#include "context.h"
#include "uistrategies.h"
#include "uimanager.h"




int main(int argc, char *argv[])
{
    Application app(argc, argv);

    typedef ctx::Bind0<strategy::DisplayApp> DisplayAppStrategy;
    QObject::connect(&app, SIGNAL(displayAppRequest()),
            new DisplayAppStrategy(&app), SLOT(exec()));

    return app.exec();
}
