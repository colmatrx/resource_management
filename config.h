#define max_number_of_processes 18  //maximum number of processes allowed in the system
#define oss_run_timeout 120 //time for the oss process to wait before killing all child processes and freeing up resources
#define processtablekey 110221 
#define resourceTableKey 112721
#define ossclockkey 1980725
#define messageqkey 1302456
#define pcbsemaphorekey 202015
void logmsg(char *filename, const char *msg);