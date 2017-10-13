#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <linux/tcp.h>
#include <sys/time.h>
#include <time.h>
#include "open62541.h"

#define OPCUA_SERVER_PORT 16664
UA_Server *server;
typedef struct _DATA_SOURCE{
	char* name;
	float data;
}DATA_SOURCE;

DATA_SOURCE source[]={
	{"press1",5.31},
	{"press2",10.5},
	{"temp1",25.25},
	{"temp2",30.1}
};

UA_Boolean running = true;
static void stopHandler(int sig) {
	running = false;
}

/*通过名字读数据*/
void* nodeIdFindData(const UA_NodeId nodeId)
{
	int i;
	for(i=0;i<sizeof(source)/sizeof(DATA_SOURCE);i++) {
		if(strncmp((char*)nodeId.identifier.string.data, source[i].name, strlen(source[i].name)) == 0) 
			return &source[i].data;
		}			
	printf("not find:%s!\n",nodeId.identifier.string.data);
	return NULL;
}

static UA_StatusCode
readFloatDataSource(void *handle, const UA_NodeId nodeId, UA_Boolean sourceTimeStamp,
                const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }		
	UA_Float currentFloat;

	if(nodeIdFindData(nodeId) != NULL)
		currentFloat = *(UA_Float*)nodeIdFindData(nodeId);
	else 
		currentFloat = -1;
	value->sourceTimestamp = UA_DateTime_now();
	value->hasSourceTimestamp = true;
    UA_Variant_setScalarCopy(&value->value, &currentFloat, &UA_TYPES[UA_TYPES_FLOAT]);
	value->hasValue = true;
	return UA_STATUSCODE_GOOD;
}

void add_dataSource_to_opcServer()
{
	int i;
	for(i=0;i<sizeof(source)/sizeof(DATA_SOURCE);i++) {
			UA_DataSource dateDataSource = (UA_DataSource) {.handle = NULL, .read = readFloatDataSource, 
				.write = NULL};
			UA_VariableAttributes *attr_float = UA_VariableAttributes_new();
    		UA_VariableAttributes_init(attr_float);
			
			attr_float->description = UA_LOCALIZEDTEXT("en_US",source[i].name);
	    	attr_float->displayName = UA_LOCALIZEDTEXT("en_US",source[i].name);
			attr_float->accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
	    	UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, source[i].name);
	    	UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, source[i].name);
	   		UA_NodeId parentNodeId = UA_NODEID_NUMERIC(1, 1000);
	   		UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
			UA_Server_addDataSourceVariableNode(server, myIntegerNodeId,parentNodeId,
	                              					parentReferenceNodeId, myIntegerName,
                                                UA_NODEID_NULL, *attr_float, dateDataSource, NULL);		
	}	
}

void handle_opcua_server(){

    UA_ServerConfig config = UA_ServerConfig_standard;
    UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, OPCUA_SERVER_PORT);
    config.networkLayers = &nl;
    config.networkLayersSize = 1;
    server = UA_Server_new(config);

    UA_ObjectAttributes object_attr;
    UA_ObjectAttributes_init(&object_attr);
    object_attr.description = UA_LOCALIZEDTEXT("en_US", "profibus");
    object_attr.displayName = UA_LOCALIZEDTEXT("en_US", "profibus");
    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, 1000),
        UA_NODEID_NUMERIC(0, 2253),
        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(1, "profibus"),
        UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), object_attr, NULL, NULL);

	add_dataSource_to_opcServer();
	
    UA_Server_run(server, &running);
		
    UA_Server_delete(server);
    nl.deleteMembers(&nl);  
}

int main(){

	//pthread_t opcua_server_id;

	//pthread_create(&opcua_server_id,NULL,(void *)handle_opcua_server,NULL);
	handle_opcua_server();
	while(1) {
		sleep(1);
	}
	return 0;
}