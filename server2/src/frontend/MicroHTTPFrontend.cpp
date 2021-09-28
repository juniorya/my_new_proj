//
//
//

#include "frontend.h"

#include <stdint.h>
#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>

extern "C" {
void process_file(FILE *pSrc, FILE *pDst);
};

static enum MHD_Result ahc_echo(void * cls,
		    struct MHD_Connection * connection,
		    const char * url,
		    const char * method,
		    const char * version,
		    const char * upload_data,
		    size_t * upload_data_size,
            void ** ptr);

static enum MHD_Result process_chart_upload_data (void *cls,
	     enum MHD_ValueKind kind,
	     const char *key,
	     const char *filename,
	     const char *content_type,
	     const char *transfer_encoding,
	     const char *data,
	     uint64_t off,
	     size_t size);

struct Session;

class CMicroHTTPFrontend: public CFrontend
{
	protected:
		struct MHD_Daemon * m_daemon;
                struct MHD_Response *m_file_not_found_response;
                struct MHD_Response *m_request_refused_response;
                const char *m_pcStaticDir;
		int m_iPort;

                struct Session *m_sessions;

	public:
		CMicroHTTPFrontend(/*CFiscalMachinesManager*/void *p, const char *pcStaticDir, int iPort);
		virtual ~CMicroHTTPFrontend();

		bool initialize();
		void deinitialize();

                Session *get_session(struct MHD_Connection *connection);

friend enum MHD_Result ahc_echo(void * cls,
    struct MHD_Connection * connection,
    const char * url,
    const char * method,
    const char * version,
    const char * upload_data,
    size_t * upload_data_size,
            void ** ptr);

friend enum MHD_Result process_chart_upload_data (void *cls,
	     enum MHD_ValueKind kind,
	     const char *key,
	     const char *filename,
	     const char *content_type,
	     const char *transfer_encoding,
	     const char *data,
	     uint64_t off,
	     size_t size);

};

TFrontend *create_HTTP_frontend(/*CFiscalMachinesManager*/void *p, const char *pcStaticDir, int iPort) {
	return new CMicroHTTPFrontend(p, pcStaticDir, iPort);
}

#define COOKIE_NAME "session"

#define PAGE "<html><head><title>libmicrohttpd demo</title>"\
             "</head><body>libmicrohttpd demo12345</body></html>"

#define FILE_NOT_FOUND_PAGE "<html><head><title>File not found</title></head><body>File not found</body></html>"

#define REQUEST_REFUSED_PAGE "<html><head><title>Request refused</title></head><body>Request refused (file exists?)</body></html>"

#define STATIC_URI_PART "static/"

#define GET_LOADED_URI "api/loaded/"

//#define MAGIC_HEADER_SIZE (16 * 1024)

#define MAXNAMESIZE 20480
#define MAXANSWERSIZE 20480
#define POSTBUFFERSIZE 20480

#ifndef uint64_t
typedef long unsigned int 	uint64_t;
#endif

struct connection_info_struct
{
  int connectiontype;
  //char *answerstring;
  CMicroHTTPFrontend *pParent;

  struct MHD_PostProcessor *postprocessor;
  union {
    std::string *p_postdata;
    void *p_custom;
  } payload;
};

/*static int iterate_post (void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
              const char *filename, const char *content_type,
              const char *transfer_encoding, const char *data,
	      uint64_t off, size_t size) {
	connection_info_struct *con_info = (connection_info_struct *)coninfo_cls;

printf("key = %s\n", key);
	if (0 == strcmp (key, "name")) {
      if ((size > 0) && (size <= MAXNAMESIZE))
        {
          char *answerstring;
          answerstring = (char *)malloc (MAXANSWERSIZE);
          if (!answerstring) return MHD_NO;

          //snprintf (answerstring, MAXANSWERSIZE, greatingpage, data);
          strcpy(answerstring, "blahblahblah");
          con_info->answerstring = answerstring;
        }
      else con_info->answerstring = NULL;

      return MHD_NO;
	}

	return MHD_YES;
}
*/

/**
 * Chart uploading context
 */
struct ChartUploadContext
{
  /**
   * Handle where we write the uploaded file to.
   */
  int fd;

  /**
   * Name of the file on disk (used to remove on errors).
   */
  char *filename;

  /**
   * Handle to connection that we're processing the upload for.
   */
  struct MHD_Connection *connection;

  /**
   * Response to generate, NULL to use directory.
   */
  struct MHD_Response *response;
};

/**
 * State we keep for each user/session/browser.
 */
struct Session
{
  /**
   * We keep all sessions in a linked list.
   */
  struct Session *next;

  /**
   * Unique ID for this session.
   */
  char sid[33];

  /**
   * Reference counter giving the number of connections
   * currently using this session.
   */
  unsigned int rc;

  time_t start;

  // ? some info for last uploaded file... maybe filename
  char last_uploaded_chart_name[32];
};


/**
 * Add header to response to set a session cookie.
 *
 * @param session session to use
 * @param response response to modify
 */
static void add_session_cookie (struct Session *session,
                    struct MHD_Response *response)
{
  char cstr[256];
  snprintf (cstr,
            sizeof (cstr),
            "%s=%s; Path=/",
            COOKIE_NAME,
            session->sid);
  if (MHD_NO ==
      MHD_add_response_header (response,
                               MHD_HTTP_HEADER_SET_COOKIE,
                               cstr))
  {
    fprintf (stderr,
             "Failed to set session cookie header!\n");
  }
}

void request_completed (void *cls, struct MHD_Connection *connection,
		void **con_cls, enum MHD_RequestTerminationCode toe) {

	connection_info_struct *con_info = (connection_info_struct *)(*con_cls);
        ChartUploadContext *uc;

	if (NULL == con_info) return;
	// 1 is POST
	if (con_info->connectiontype == 1) {
		//MHD_destroy_post_processor (con_info->postprocessor);
		//if (con_info->answerstring) free (con_info->answerstring);
		if (con_info->payload.p_postdata) delete con_info->payload.p_postdata;
	} else
	if (con_info->connectiontype == 2) {
            // this is our's chart uploading

            if (NULL != con_info->postprocessor) {
                MHD_destroy_post_processor (con_info->postprocessor);
                con_info->postprocessor = NULL;
            }

            uc = (ChartUploadContext *)(con_info->payload.p_custom);
            if (-1 != uc->fd) {
                (void) close (uc->fd);
                if (NULL != uc->filename) {
                    fprintf (stderr,
                    "Upload of file `%s' failed (incomplete or aborted), removing file.\n",
                    uc->filename);
                    (void) unlink (uc->filename);
                }
            }
            if (NULL != uc->filename)  free (uc->filename);

            if (con_info->payload.p_custom) free(con_info->payload.p_custom);
        }

//if (NULL != request->session)
//    request->session->rc--;

	free (con_info);
	*con_cls = NULL;
}


/**
 * Iterator over key-value pairs where the value
 * maybe made available in increments and/or may
 * not be zero-terminated.  Used for processing
 * POST data.
 *
 * @param cls user-specified closure
 * @param kind type of the value, always MHD_POSTDATA_KIND when called from MHD
 * @param key 0-terminated key for the value
 * @param filename name of the uploaded file, NULL if not known
 * @param content_type mime-type of the data, NULL if not known
 * @param transfer_encoding encoding of the data, NULL if not known
 * @param data pointer to size bytes of data at the
 *              specified offset
 * @param off offset of data in the overall value
 * @param size number of bytes in data available
 * @return MHD_YES to continue iterating,
 *         MHD_NO to abort the iteration
 */
static enum MHD_Result
process_chart_upload_data (void *cls,
	     enum MHD_ValueKind kind,
	     const char *key,
	     const char *filename,
	     const char *content_type,
	     const char *transfer_encoding,
	     const char *data,
	     uint64_t off,
	     size_t size)
{
  connection_info_struct *con_info = (connection_info_struct *)(cls);
  ChartUploadContext *uc = (ChartUploadContext *)(con_info->payload.p_custom);

printf("process: %hhu %s\n", con_info->connectiontype, key);
  int i;

//  if (0 == strcmp (key, "category"))
//    return do_append (&uc->category, data, size);
//  if (0 == strcmp (key, "language"))
//    return do_append (&uc->language, data, size);
  if (0 != strcmp (key, "file_proto"))
    {
      printf (/*stderr,*/
           "Ignoring unexpected form value `%s'\n",
           key);
      return MHD_YES; /* ignore */
    }
  if (NULL == filename)
    {
      printf (/*stderr,*/ "No filename, aborting upload\n");
      return MHD_NO; /* no filename, error */
    }

  if (-1 == uc->fd)
    {
      char fn[PATH_MAX];

      if ( (NULL != strstr (filename, "..")) ||
       (NULL != strchr (filename, '/')) ||
       (NULL != strchr (filename, '\\')) )
    {
      uc->response = con_info->pParent->m_request_refused_response;
      return MHD_NO;
    }

      /* open file */

/*
      snprintf (fn, sizeof (fn),
	"%s/%s/%s",
	uc->language,
	uc->category,
	filename);
*/

    uc->filename = tempnam("/tmp", "mgs2l_");
    if (uc->filename) {

// TODO: fn as temp file
/*      for (i=strlen (fn)-1;i>=0;i--)
    if (! isprint ((int) fn[i]))
      fn[i] = '_';
*/
      uc->fd = open (uc->filename,
	     O_CREAT | O_EXCL
#if O_LARGEFILE
	     | O_LARGEFILE
#endif
	     | O_WRONLY,
	     S_IRUSR | S_IWUSR);
      if (-1 == uc->fd)
    {
      printf (/*stderr, */
	   "Error opening file `%s' for upload: %s\n",
	   uc->filename,
	   strerror (errno));
      uc->response = con_info->pParent->m_request_refused_response;
      return MHD_NO;
    }
//      uc->filename = strdup (fn);
    } else {
printf("Error get temporary file\n");
      uc->response = con_info->pParent->m_request_refused_response;
    }
  }
  if ( (0 != size) &&
       (size != (size_t) write (uc->fd, data, size)) )
    {
      /* write failed; likely: disk full */
      printf (/*stderr, */
           "Error writing to file `%s': %s\n",
           uc->filename,
           strerror (errno));
//      uc->response = internal_error_response;
      uc->response = con_info->pParent->m_request_refused_response;


      (void) close (uc->fd);
      uc->fd = -1;
      if (NULL != uc->filename)
    {
      unlink (uc->filename);
      free (uc->filename);
      uc->filename = NULL;
    }
      return MHD_NO;
    }
  return MHD_YES;
}


static enum MHD_Result ahc_echo(void * cls,
		    struct MHD_Connection * connection,
		    const char * url,
		    const char * method,
		    const char * version,
		    const char * upload_data,
		    size_t * upload_data_size,
            void ** ptr) {
	static int dummy;

	CMicroHTTPFrontend *pF =  (CMicroHTTPFrontend *)cls;
	struct MHD_Response * response;
        struct Session *session = 0;
	//int ret;
        enum MHD_Result ret;

	// *ptr == 0 while initialization call
	if (*ptr == 0) {
		connection_info_struct *con_info;
		con_info = (connection_info_struct *) malloc ((unsigned int)sizeof (struct connection_info_struct));
		if (!con_info) return MHD_NO;
                con_info->pParent = pF;
		con_info->payload.p_postdata = 0;
		con_info->connectiontype = 0;
                con_info->postprocessor = 0;

printf("%s %s\n", method, url);

		if (strcmp(method, "POST") == 0) {
                    if (strcmp(url, "/loader") == 0) {
printf("Loader!\n");
                        con_info->payload.p_custom = malloc ((unsigned int)sizeof (struct ChartUploadContext));
                        ChartUploadContext *uc = (ChartUploadContext *)(con_info->payload.p_custom);

                        uc->fd = -1;
                        uc->filename = 0;
                        uc->response = 0;

                        con_info->postprocessor = MHD_create_post_processor (connection,
                            8 * 1024 /* buffer size */,
                            &process_chart_upload_data, con_info);

			con_info->connectiontype = 2;
                    } else {

//printf("post1\n");
//printf("upload data size = %d\n", *upload_data_size);

//			con_info->postprocessor = MHD_create_post_processor (connection, POSTBUFFERSIZE, iterate_post, (void*) con_info);

//			if (!con_info->postprocessor) {
//printf("post - shit\n");
//				free (con_info);
//				return MHD_NO;
//			}
//printf("post2\n");
			con_info->connectiontype = 1;
                    }
		} else con_info->connectiontype = 0; // GET
		*ptr = (void *)con_info;
		return MHD_YES;
	}

	//*ptr = NULL; // clear context pointer

	bool bAddJsonHeader = false;
	std::string s_res;
	const char *pcResult = 0;
	unsigned int uResultCode = MHD_HTTP_OK;
	std::string s_command((strlen(url) == 0)? "" : url + 1);


        session = pF->get_session (connection);

	if (strcmp(method, "POST") == 0) {
		connection_info_struct *con_info = (connection_info_struct *)(*ptr);

//printf("upload data size = %d\n", *upload_data_size);
                unsigned long loaded = 0;
                if (con_info->connectiontype == 1) {
		    loaded = (con_info->payload.p_postdata == 0)? 0 : con_info->payload.p_postdata->size();
		    if ((loaded + (*upload_data_size)) > POSTBUFFERSIZE) {
			uResultCode = MHD_HTTP_INTERNAL_SERVER_ERROR;
			pcResult = "buffer overflow";
		    }
                }

		if (*upload_data_size != 0) {
                    if (con_info->connectiontype == 1) {
			//MHD_post_process (con_info->postprocessor, upload_data, *upload_data_size);
			    if (con_info->payload.p_postdata == 0) con_info->payload.p_postdata = new std::string();
			    con_info->payload.p_postdata->append(upload_data, *upload_data_size);
                    } else {
                        //if (NULL == uc->response)
                        (void) MHD_post_process (con_info->postprocessor,
		        upload_data,
		        *upload_data_size);
                    }
		    *upload_data_size = 0;
		    return MHD_YES;
		} else
                if (con_info->connectiontype > 0) {//		if (NULL != con_info->p_postdata)
			// we've got the POST

//printf("POST!!! %s\n", con_info->p_postdata->c_str());
                        if (con_info->connectiontype == 1) {
			    Json::Value in, out;
			    bool bJsonGood = true;
			    try {
				Json::Reader reader;
				reader.parse(*(con_info->payload.p_postdata), in);
			    }
			    catch(...) {
				uResultCode = MHD_HTTP_INTERNAL_SERVER_ERROR;
				pcResult = "bad json";
				bJsonGood = false;
			    }

			    if (bJsonGood) {
				int iFRes = pF->handleRequest(s_command.c_str(), in, out);
				if ((iFRes == FRONTEND_OK) || (iFRes == FRONTEND_ERROR)) {
					// dump out to string
					Json::StyledWriter writer;
					s_res = writer.write(out);
					bAddJsonHeader = true;
					uResultCode = (iFRes == FRONTEND_OK)? MHD_HTTP_OK : MHD_HTTP_INTERNAL_SERVER_ERROR;
				} else {
					// TODO: return some codes like "not implemented", "forbidden" etc
					//uResultCode = MHD_HTTP_INTERNAL_SERVER_ERROR;
					uResultCode = MHD_HTTP_NOT_FOUND;
				}
				pcResult = s_res.c_str();
			    }
                        } else {
                            // it was chart uploading
// TODO: duplication in request_completed

                            if (NULL != con_info->postprocessor) {
                                MHD_destroy_post_processor (con_info->postprocessor);
                                con_info->postprocessor = NULL;
                            }

                            ChartUploadContext *uc = (ChartUploadContext *)(con_info->payload.p_custom);
                            if (NULL != uc->filename)  {

// ? maybe set filename in heap (just pointer to cookie)? and then purge procedure
                                strncpy(session->last_uploaded_chart_name, uc->filename, sizeof(Session::last_uploaded_chart_name) - 1);

printf("Copied %s\n", session->last_uploaded_chart_name);

                                free (uc->filename);
                                uc->filename = 0;
                            }
                            if (-1 != uc->fd) {
                                close (uc->fd);
                                uc->fd = -1;
                            }
                            if (NULL != uc->response) {

                                MHD_Response *r = uc->response;
                                if (con_info->payload.p_custom) free(con_info->payload.p_custom);

                                return MHD_queue_response(connection, MHD_HTTP_FORBIDDEN, r);
                            }
                            if (con_info->payload.p_custom) free(con_info->payload.p_custom);

                            bAddJsonHeader = 1;
                            pcResult = "{ \"success\": true }";
                        }
		} else {
			pcResult = "error";
			uResultCode = MHD_HTTP_INTERNAL_SERVER_ERROR;
		}
	} else {
		if (0 != *upload_data_size)
			return MHD_NO; /* upload data in a GET!? */

		uResultCode = MHD_HTTP_NOT_FOUND;

		if (s_command.size() == 0) {
			// root '/'
			uResultCode = MHD_HTTP_FORBIDDEN;
			pcResult = "";
		} else
		if (s_command.compare("testp") == 0) {
			//const char *p = PAGE;
			//s_res.append(p);
			pcResult = PAGE;
			uResultCode = MHD_HTTP_OK;
		} else
                if ((s_command.size() > strlen(STATIC_URI_PART)) && (!strncmp(s_command.c_str(), STATIC_URI_PART, strlen(STATIC_URI_PART)))) {

                    // we have to return a file from static directory
                    const char *pcFilename = s_command.c_str() + strlen(STATIC_URI_PART);
printf("Req %s\n", pcFilename);

//                    char file_data[MAGIC_HEADER_SIZE];
                    ssize_t got;
                    const char *mime;
                    int fd;
                    struct stat buf;

                    std::string filename(pF->m_pcStaticDir);
                    filename.append(pcFilename);

                    if ((0 == stat (filename.c_str(), &buf)) &&
                        (NULL == strstr (s_command.c_str(), ".."))) {

printf("fn %s\n", filename.c_str());

                        fd = open (filename.c_str(), O_RDONLY);
                    } else
                        fd = -1;

                    if (-1 == fd)
                        return MHD_queue_response (connection,
                           MHD_HTTP_NOT_FOUND,
                           pF->m_file_not_found_response);

                    /* read beginning of the file to determine mime type  */
//                    got = read (fd, file_data, sizeof (file_data));
//                    if (-1 != got)
//                        mime = magic_buffer (magic, file_data, got);
//                    else
                        mime = NULL;
//                    (void) lseek (fd, 0, SEEK_SET);

                    if (NULL == (response = MHD_create_response_from_fd (buf.st_size, fd))) {
                        /* internal error (i.e. out of memory) */
                        (void) close (fd);
                        return MHD_NO;
                    }

                    add_session_cookie (session, response);
                    /* add mime type if we had one */
                    if (NULL != mime)
                        (void) MHD_add_response_header (response,
                            MHD_HTTP_HEADER_CONTENT_TYPE,
                            mime);

                    ret = MHD_queue_response (connection,
                        MHD_HTTP_OK,
                        response);

                    MHD_destroy_response (response);
                    return ret;

		} else
                if (!strcmp(s_command.c_str(), GET_LOADED_URI)) {
                    // TODO: MAYBE move out of HTTP frontend
                    if (session->last_uploaded_chart_name[0] != 0) {

printf("Will gen from %s\n", session->last_uploaded_chart_name);
printf("0\n");
                        FILE *pSrc = fopen(session->last_uploaded_chart_name, "rb");
printf("1\n");
                        
                        //int iFD = mkstemp("/tmp/parsedXXXXXX");
                    char *pTmp = tempnam("/tmp", "parsed_");
                    int iFD = open (pTmp, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
printf("2\n");
                        FILE *pDst = fdopen(iFD, "w+");
printf("3\n");
                        process_file(pSrc, pDst);
printf("4\n");
                        fclose(pSrc);
printf("5\n");

                        unsigned uLen = ftell(pDst);
printf("Len=%u\n", uLen);
                        fseek (pDst, 0, SEEK_SET);

                        if (NULL == (response = MHD_create_response_from_fd (uLen, iFD))) {
                            /* internal error (i.e. out of memory) */
                            (void) fclose (pDst);
                            return MHD_NO;
                        }

                        add_session_cookie (session, response);
                        MHD_add_response_header(response, "Content-Type", "application/json; charset=utf-8");

                        ret = MHD_queue_response (connection,
                            MHD_HTTP_OK,
                            response);

                        MHD_destroy_response (response);
//? fclose ???
                        return ret;
                    } else {
                        pcResult = "";
			uResultCode = MHD_HTTP_NOT_FOUND;
                    }
                } else {
			// will parse json when will process POST
			Json::Value in, out;
			int iFRes = pF->handleRequest(s_command.c_str(), in, out);
			if ((iFRes == FRONTEND_OK) || (iFRes == FRONTEND_ERROR)) {
				// dump out to string
				Json::StyledWriter writer;
				s_res = writer.write(out);
				bAddJsonHeader = true;
				uResultCode = (iFRes == FRONTEND_OK)? MHD_HTTP_OK : MHD_HTTP_INTERNAL_SERVER_ERROR;
			} else {
				// TODO: return some codes like "not implemented", "forbidden" etc
				//uResultCode = MHD_HTTP_INTERNAL_SERVER_ERROR;
				uResultCode = MHD_HTTP_NOT_FOUND;
			}
			pcResult = s_res.c_str();
		}
	}

//printf("%d %s\n", strlen(pcResult), pcResult);

	/*	response = MHD_create_response_from_buffer ((unsigned long int)s_res.size(),
												  (void*)s_res.c_str(),
							  MHD_RESPMEM_PERSISTENT);
	*/	response = MHD_create_response_from_data ((unsigned long int)strlen(pcResult),
												  (void*)pcResult, 0, 1);
        add_session_cookie (session, response);
	if (bAddJsonHeader) MHD_add_response_header(response, "Content-Type", "application/json; charset=utf-8");

	ret = MHD_queue_response(connection,
			   uResultCode,
			   response);
	MHD_destroy_response(response);
	return ret;
}

CMicroHTTPFrontend::CMicroHTTPFrontend(/*CFiscalMachinesManager*/void *p, const char *pcStaticDir, int iPort) : 
    /*CFrontend(p),*/ m_daemon(0), m_pcStaticDir(pcStaticDir), m_iPort(iPort), m_sessions(0)
{
    m_file_not_found_response = MHD_create_response_from_buffer (strlen (FILE_NOT_FOUND_PAGE),
			         (void *) FILE_NOT_FOUND_PAGE,
			         MHD_RESPMEM_PERSISTENT);
    m_request_refused_response = MHD_create_response_from_buffer (strlen (REQUEST_REFUSED_PAGE),
			         (void *) REQUEST_REFUSED_PAGE,
			         MHD_RESPMEM_PERSISTENT);
}

CMicroHTTPFrontend::~CMicroHTTPFrontend()
{
}

bool CMicroHTTPFrontend::initialize()
{
	m_daemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION,
			       m_iPort,
			       NULL,
			       NULL,
			       &ahc_echo,
			       this,
			       MHD_OPTION_END);
	return (m_daemon == NULL)? false : true;
}

void CMicroHTTPFrontend::deinitialize()
{
	if (m_daemon) MHD_stop_daemon(m_daemon);
    MHD_destroy_response (m_file_not_found_response);
    MHD_destroy_response (m_request_refused_response);
//    MHD_destroy_response (m_internal_error_response);
}

Session *CMicroHTTPFrontend::get_session(struct MHD_Connection *connection)
{
  struct Session *ret;
  const char *cookie;

  cookie = MHD_lookup_connection_value (connection,
                                        MHD_COOKIE_KIND,
                                        COOKIE_NAME);
  if (cookie != NULL)
  {
    /* find existing session */
    ret = m_sessions;
    while (NULL != ret)
    {
      if (0 == strcmp (cookie, ret->sid))
        break;
      ret = ret->next;
    }
    if (NULL != ret)
    {
      ret->rc++;
      return ret;
    }
  }
  /* create fresh session */
  ret = (Session *)calloc (1, sizeof (struct Session));
  if (NULL == ret)
  {
    fprintf (stderr, "calloc error: %s\n", strerror (errno));
    return NULL;
  }
  /* not a super-secure way to generate a random session ID,
     but should do for a simple example... */
  snprintf (ret->sid,
            sizeof (ret->sid),
            "%X%X%X%X",
            (unsigned int) rand (),
            (unsigned int) rand (),
            (unsigned int) rand (),
            (unsigned int) rand ());
  ret->rc++;
  ret->start = time (NULL);
  ret->next = m_sessions;
  ret->last_uploaded_chart_name[0] = 0;
  m_sessions = ret;
  return ret;
}

