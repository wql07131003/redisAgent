#include <stdio.h>

#include <ub_log.h>
#include <ubclient_include.h>

#include "SuperStrategy.h"

using namespace forum;

UBCLIENT_GEN_IOC_CREATE(SuperStrategy);
ub::UbClientManager ubmgr;

static int talk();

int main(int /*argc*/, char ** /*argv*/)
{
	ub_log_init("./log", "demo1");
	ub::strategy_ioc *strategyIoc = ub::ubclient_get_strategyioc();
	UBCLIENT_ADD_IOC_CLASS(strategyIoc, SuperStrategy);

	int ret = ubmgr.init();
	if (0 != ret) {
		printf("%s\n", "init error");
		return -1;
	}
	ubmgr.startHealthyCheck(10);
	printf("%s\n", "init OK");

	talk();

	sleep(3);

	talk();

	sleep(3);

	talk();
	sleep(3);

	talk();

    ubmgr.cancelAll();
    ubmgr.close();
    ub_log_close();

	return 0;
}

int talk()
{
	int ret = 0;
	ub::nshead_talkwith_t *currTalk = new ub::nshead_talkwith_t;
	if (NULL == currTalk) {
		printf("new nshead_talkwith_t error\n");
		return -1;
	}
	char resMsg[] = "hello world";
	currTalk->reqhead.log_id = rand();
	printf("curr id : %d\n", currTalk->reqhead.log_id);
	printf("curr len : %d\n", (int)sizeof(resMsg));
	currTalk->reqbuf = new char[100];
	memset(currTalk->reqbuf, 0, 100);
	currTalk->reqhead.body_len = snprintf(currTalk->reqbuf, 100, "%s", resMsg);
	printf("curr msg : %s; curr len : %d\n", currTalk->reqbuf, (int)strlen(currTalk->reqbuf));

	currTalk->resbuf = new char[100];
	memset(currTalk->resbuf, 0, 100);
	currTalk->maxreslen = 100;
	currTalk->talkwithcallback = NULL;
	currTalk->alloccallback = NULL;
	currTalk->freecallback = NULL;
	currTalk->allocarg = NULL;
	currTalk->success = 0;

	ubmgr.readLock("service1");
	bsl::var::Null currDict;
	currTalk->reqhead.log_id = rand();
	currTalk->reqhead.body_len = snprintf(currTalk->reqbuf, 100, "%s", resMsg);
	ret = ubmgr.nshead_singletalk("service1", currTalk, currDict);
	//ret = ubmgr.nshead_nonblock_singletalk("service1", currTalk, currDict);
	ubmgr.unlock("service1");
	if (ret < 0) {
		printf("talk start over\n");
		goto Fail;
	} else if (currTalk->success == -1) {
		printf("%s\n", "currTalk failed!");
	} else {
		printf("talk OK\n");
	}
	//return 0;

	ret = ubmgr.wait(100);
	if (ret > 0) {
		ub::nshead_talkwith_t *curr = ubmgr.fetchFirstReadyReq();
		if (curr->success == 0) {
			printf("OK!!!!!!!!!!!!!!11");
			printf("%s\n(((((((((((((((((( \n", curr->resbuf);

			printf("*******time read: %d, write: %d\n", curr->returninfo.realreadtime, curr->returninfo.realwritetime);

		} else {
			printf("Error!!!!!!!!!!!!");
		}
		printf("%s\n", "currTalk talk over");
	} else {
		printf("%s\n", "wait failed");
		if (currTalk->success == -1) {
			ub::print_talk_errorinfo(currTalk->success);
		}
		ubmgr.cancelAll();
	}

	ubmgr.cancelAll();
	ubmgr.close();
	ub_log_close();
	delete [] currTalk->resbuf;
	delete [] currTalk->reqbuf;
	delete currTalk;
	return 0;

Fail:
    delete [] currTalk->resbuf;
    delete [] currTalk->reqbuf;
    delete currTalk;

    return 0;
}

