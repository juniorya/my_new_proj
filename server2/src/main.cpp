//
// blah
//

//#include "config/config.h"
//#include "Manager.h"
#include "frontend/frontend.h"
#include "JSON/json/json.h"

#include <string.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h> /* getprotobyname */
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/termios.h>
#include <errno.h>
#include <linux/limits.h>

extern "C" {
    int find_yourself(char *argv0, char *result, size_t size_of_result);
    int yourself_get_directory(const char *pcMe, char *result, size_t size_of_result);
};

//#include "arch/AsynchroLibUSBDevice.h"

int funcc() {
    /* make stdin non-blocking */
    {
        int f;

        // fetch the current flags
        if ((f = fcntl(STDIN_FILENO, F_GETFL, 0)) == -1) {
            return 1; //err(1, "fcntl(GETFL)");
        }
        // now set the flags to what they are + non-blocking
        if ((f = fcntl(STDIN_FILENO, F_SETFL, f | O_NONBLOCK)) == -1) {
            return 1; //err(1, "fcntl(SETFL)");
        }
    }

    /* try to unbuffer our terminal (if indeed that is what stdin is connected
      to) */
    struct termios termios;

    {

        // get our current terminal settings
        if (tcgetattr(STDIN_FILENO, &termios) != 0) {
            // NB if stdin isn't a terminal (e.g. we are reading from a file
            // or a pipe) then this will fail. good software wouldn't exit in
            // that case - luckily we are just example code.
            return 1; //err(1, "tcgetattr");
        }
        // when we exit we need to restore the terminal the way it was rather
        // than leave it in a broken state. most shells will actually clean up
        // after us so this won't always be needed but best to be polite.
/*        atexit_b(^{
                tcsetattr(STDIN_FILENO, TCSASOFT, &termios);
                // we don't error check this becuase if it fails there isn't
                // anything we can do about it
            });
*/
        // build a description of the mode we would like the terminal to be in
        // by starting with the current settings and changing a few things.
        // first switch of canonical mode and echo. switching off echo isn't
        // actually required but often wanted. switching off canonical mode
        // means we don't have to wait for a newline to be able to read
        termios.c_lflag &= ~(ICANON | ECHO);
        // then make sure read will return straight away
        termios.c_cc[VMIN] = 0;
        termios.c_cc[VTIME] = 0;
        // now apply those settings to the terminal
        if (tcsetattr(STDIN_FILENO, 0x10 /*TCSASOFT*/, &termios) != 0) {
            return 1; //err(1, "tcsetattr");
        }
    }

    /* now it should be safe for us to read from the terminal without
      blocking. to prevent us spinning we use select to wait till there is
      (probably) something to read */
    {
        fd_set fd_set;

bool bNeedExit = false;
        while (!bNeedExit) {
            // select needs a set of descriptors we want to know about. that's
            // only 1 in this trivial example
            FD_ZERO(&fd_set);
            FD_SET(STDIN_FILENO, &fd_set);

            // now call select. NB the first argument is the maximum fd + 1 -
            // not the number of fds in the set. our timeout is NULL so
            // select should never return 0. we should block here until there
            // is data to be read
            if (select(STDIN_FILENO + 1, &fd_set, NULL, NULL, NULL) == -1) {
                if (errno == EINTR) {
                    // this isn't really an error so just try again
                    continue;
                } else {
                    return 1; //err(1, "select");
                }
            }

            // check if the fd of interest is in the set of read fds (we only
            // have one so this is very unlikely to ever be false). the set contains
            // the gds that can be read from without blocking
            if (FD_ISSET(STDIN_FILENO, &fd_set)) {
                char buffer[32];
                int n;

                // we should have something to read from stdin so try it - it may
                // be more than 1 byte though unlikely if coming from a keyboard
                // we should, but don't, check for read returning all 32 bytes as
                // that could mean we should call read again without looping
                // through the select again (just for efficiency)
                switch (n = read(STDIN_FILENO, (void *)buffer, sizeof(buffer))) {
                    case -1:
                        if (errno == EAGAIN) {
                            // there wasn't anything to read after all
                            continue;
                        } else {
                            return 1; //err(1, "read");
                        }
                        break;
                    case 0:
                        // EOF
                        //exit(0);
bNeedExit = true;
                        break;
                    default:
                        // print out what we read
                        printf("Read: %*s\n", n, buffer);
                        break;
                }
            } else {
                // we only had one fd in the set yet "another" fd caused
                // select to return. I doubt this will ever happen (famous
                // last words)
            //    warnx("select returned with unknown fd");
            }
        }
    }

tcsetattr(STDIN_FILENO, /*TCSASOFT*/ 0x10, &termios);

return 0;
}

int mygetch( ) {
  struct termios oldt, newt;
  int            ch;
  tcgetattr( STDIN_FILENO, &oldt );
  newt = oldt;
  newt.c_lflag &= ~( ICANON | ECHO );
  tcsetattr( STDIN_FILENO, TCSANOW, &newt );
  ch = getchar();

/*
    fd_set fd_set;

bool bNeedExit = false;
        while (!bNeedExit) {
            FD_ZERO(&fd_set);
            FD_SET(STDIN_FILENO, &fd_set);

            if (select(STDIN_FILENO + 1, &fd_set, NULL, NULL, NULL) == -1) {
                if (errno == EINTR) {
                    // this isn't really an error so just try again
                    continue;
                } else {
                    return 1; //err(1, "select");
                }
            }
printf("cycling\n");
            if (FD_ISSET(STDIN_FILENO, &fd_set)) {
                char buffer[32];
                int n;

                switch (n = read(STDIN_FILENO, (void *)buffer, sizeof(buffer))) {
                    case -1:
                        if (errno == EAGAIN) {
                            // there wasn't anything to read after all
                            continue;
                        } else {
                            return 1; //err(1, "read");
                        }
                        break;
                    case 0:
                        // EOF
                        //exit(0);
bNeedExit = true;
                        break;
                    default:
                        // print out what we read
                        printf("Read: %*s\n", n, buffer);
                        break;
                }
            } else {
                // we only had one fd in the set yet "another" fd caused
                // select to return. I doubt this will ever happen (famous
                // last words)
            //    warnx("select returned with unknown fd");
            }
        }
*/
  tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
  return ch;
}

//bool initialize_config_from_conf(const char *pszConfigFile);

char pcMe[PATH_MAX];
char pcStaticDir[PATH_MAX + 8];  // 8 is the length of "/static/"

int main(int argc, char *argv[])
{
    if (find_yourself(argv[0], pcMe, PATH_MAX)) {
        printf("Locator error.\n");
        return 1;
    }

    if (yourself_get_directory(pcMe, pcStaticDir, PATH_MAX)) {
        // in theory it could be root, but safer is to exclude this situation
        printf("Locator error 2.\n");
        return 1;
    }
    strcat(pcStaticDir, "/static/");

    srand(time(0));

//funcc();
//return 0;

//	if (!initialize_config_from_conf("kakatdaemon.conf")) return 1;

//	CConfig *pC = CConfig::get_instance();
	/*Json::Value fm = get_machine_config("7724906184", "02001894");
	if (!fm.isNull()) {
		Json::StyledWriter writer;
		std::string s_res = writer.write(fm);
		printf("%s\n", s_res.c_str());
	}
	*/

//	CFiscalMachinesManager *pManager = new CFiscalMachinesManager();
//	pManager->initialize();

//	TMedia *pUSB = create_USB_media(pManager);
//	pUSB->initialize();

	TFrontend *pF = 0;
//	Json::Value http = pC->get_from_scope_with_value(0, "frontends", "type", "http");
//	if ((!http.isNull()) && (http.isMember("port"))) {
//		std::string sPort = http["port"].asString();
//		int iPort = atoi(sPort.c_str());
int iPort = 8000;
		pF = create_HTTP_frontend(/*pManager*/ 0, pcStaticDir, iPort);
		pF->initialize();
//	}

    printf("Daemon started. Press Q to quit");
    while(true) {
    		char c = mygetch();
    		if ((c == 'q') || (c == 'Q')) break;
    	}

    if (pF) pF->deinitialize();
//    pUSB->deinitialize();
//    pManager->deinitialize();
    delete pF;
//    delete pUSB;
//    delete pManager;

    return 0;
}
