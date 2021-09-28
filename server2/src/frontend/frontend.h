//
//
//

#pragma once

#include "../JSON/json/json.h"
class CFiscalMachinesManager;

class TFrontend {
	public:
		virtual ~TFrontend() {};

		virtual bool initialize() = 0;
		virtual void deinitialize() = 0;

	// maybe some statistics? or reconnection ?
};

#define FRONTEND_OK 0
#define FRONTEND_NOT_FOUND 404
#define FRONTEND_ERROR 500

class CFrontend: public TFrontend
{
	protected:
//	CFiscalMachinesManager *m_pParent;
	public:
//		CFrontend(CFiscalMachinesManager *p) : m_pParent(p) {}
		CFrontend() {}
		virtual ~CFrontend()  {}

		bool initialize() { return true; }
		void deinitialize() { }

		int handleRequest(const char *pszCommand, Json::Value &in, Json::Value &out);
};

// TODO: Fabric function. Maybe move it out of here

TFrontend *create_HTTP_frontend(/*CFiscalMachinesManager*/ void *p, const char *pcStaticDir, int iPort);
