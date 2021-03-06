/*
  Persistent Object Store

  Author: Taeho Hwang (htaeh@hanynag.ac.kr)
  Embedded Software Systems Laboratory, Hanyang University
*/

#include <stdio.h>
#include <string.h>
#include <pos-lib.h>
#include <pthread.h> 

#include <unistd.h>
#include <syscall.h>


#define LOG_SIZE_KB			64		// Unit of KByte (Must be multiple of 16)
#define LOG_SIZE				(LOG_SIZE_KB*1024)
#define T_NAME			"TS"

#define KV_SIZE			(16*1024)

#define KV_BASE			(2)
#define MALLOC_BASE		(KV_SIZE/sizeof(unsigned long))
#define MALLOC_END		(LOG_SIZE/sizeof(unsigned long)-2)


#define KEONWOO_DEBUG 0

#define LOG_CNT_ON		0
int clflush_cnt = 0;
int log_cnt = 0;
void print_log_cnt(void)
{
	printf("[POS] clflush_cnt = %d\n", clflush_cnt);
	printf("[POS] log_cnt = %d\n", log_cnt);
}
void clear_log_cnt(void)
{
	clflush_cnt = 0;
	log_cnt = 0;
}



/* Log structure

-----------------------------------------
< [0] STARTorEND		/ [1] INSERTorREMOVE >
-----------------------------------------
< [2] CNT_PTR		/ [3] NULL	>			// KV
< [4] ADDRESS		/ [5] VALUE	>
< [6] ADDRESS		/ [7] VALUE 	>
	... // ptr log increase below
	...
-----------------------------------------
< [2048] CNT_PTR		/ [2049] CNT_FREE>		// POS_MALLOC
< [2050] ADDRESS	/ [2051] VALUE	>
< [2052] ADDRESS	/ [2053] VALUE	>
	... // ptr log increase below
	...
	... // free(seg_free) log increase up
< [8188] ADDRESS	/ [8189] SIZE		>
< [8190] ADDRESS	/ [8191] SIZE		>
-----------------------------------------

*/

int pos_log_create(char *name)
{
	struct pos_name_entry *name_entry;
	char log_name[POS_NAME_LENGTH];

	// Check whether object storage which is target of logging is mapped or not.
	name_entry = pos_lookup_name_entry(name);
	if (name_entry == NULL) {
		return 0;
	}

	strcpy(log_name, name);
	strcat(log_name, T_NAME);

	// Call sys_pos_create() to create log object storage.
	name_entry->log_addr = (void *)syscall(299, log_name, LOG_SIZE_KB);
	if (name_entry->log_addr == NULL)
		return 0;
	
	memset(name_entry->log_addr, 0, LOG_SIZE);

	return 1;
}

int pos_log_delete(char *name)
{
	char log_name[POS_NAME_LENGTH];

	// A user-level name table entry has been deleted in pos_delete().
	
	strcpy(log_name, name);
	strcat(log_name, T_NAME);

	return syscall(300, log_name); // sys_pos_delete()
}
#include <sys/types.h> 

int pos_log_map(char *name)
{
	struct pos_name_entry *name_entry;
	char log_name[POS_NAME_LENGTH];
	#if KEONWOO_DEBUG == 1 
	printf("\n ------ pos_log_map ------ \n") ; 	
	#endif
	// Check whether object storage which is target of logging is mapped or not.
	name_entry = pos_lookup_name_entry(name);
	if (name_entry == NULL) {
		return 0;
	}// absolutely not!!.

	strcpy(log_name, name);
	strcat(log_name, T_NAME);
	// When multithreading environment,We have to call 307( sys_pos_thread_map) ; 	
	pid_t pid ; 	
	pid_t tid ; 	
	// To get Thread ID called __NR_gettid ; 	
	tid = syscall(__NR_gettid) ;
 	pid = getpid() ; 	
/*	if( tid != pid ){
		 printf("Log file in Multithreading\n") ; 	
		void * ret = syscall( 307 , log_name ) ; 	
		return 1; 
	}	*/ 	

	// Call sys_pos_map() to create log object storage.
	name_entry->log_addr = (void *)syscall(301, log_name);
	if (name_entry->log_addr == NULL)
		return 0;
	#if KEONWOO_DEBUG == 1 
	printf("------end------\n") ; 	
	#endif
	return 1;
}

int pos_log_unmap(char *name)
{
	char log_name[POS_NAME_LENGTH];

	// A user-level name table entry has been deleted in pos_unmap().
	
	strcpy(log_name, name);
	strcat(log_name, T_NAME);

	return syscall(302, log_name); // sys_pos_unmap()
}

int pos_log_clear_ptr(unsigned long *log_addr)
{

	log_addr[KV_BASE] = 0;
	#if KEONWOO_DEBUG == 1 	
	printf(" log_addr[%d] = 0\n" , KV_BASE) ; 		
	#endif
	pos_clflush_cache_range(&log_addr[KV_BASE], sizeof(unsigned long));

	log_addr[MALLOC_BASE] = 0;
	#if KEONWOO_DEBUG == 1 
	printf(" log_addr[%d] = 0\n" , MALLOC_BASE) ; 	
	#endif
	pos_clflush_cache_range(&log_addr[MALLOC_BASE], sizeof(unsigned long));
	return 1;
}

int pos_log_clear_free(unsigned long *log_addr)
{
	log_addr[MALLOC_BASE+1] = 0;
	#if KEONWOO_DEBUG == 1 
	printf(" log_addr[%d] = 0\n" , MALLOC_BASE+1) ; 		
	#endif
	pos_clflush_cache_range(&log_addr[MALLOC_BASE+1], sizeof(unsigned long));

	return 1;
}

int pos_transaction_start(char *name, unsigned long type)
{
	struct pos_name_entry *name_entry;
	unsigned long *log_addr;
	#if KEONWOO_DEBUG == 1
		printf("[POS_transaction_start]\n") ; 	
	#endif
	name_entry = pos_lookup_name_entry(name);
	log_addr = name_entry->log_addr;
	#if KEONWOO_DEBUG ==  1
		printf("[log_addr] %lu\n" , log_addr) ; 	
	#endif
	// 1. Clear counter
	#if KEONWOO_DEBUG == 1
		printf("1) pos_log_clear_ptr\n") ;	
	#endif
	pos_log_clear_ptr(log_addr);
	#if KEONWOO_DEBUG ==  1
		printf("2) pos_log_clear_free\n") ; 	
	#endif 
	pos_log_clear_free(log_addr);
	
	// 2. Make the transaction flag indicate the START.
	log_addr[0] = POS_TS_START;
	#if KEONWOO_DEBUG==  1
		printf("log_addr[0] = %d\n" , POS_TS_START) ; 
	#endif 
	pos_clflush_cache_range(&log_addr[0], sizeof(unsigned long));

	// 3. Set insert/remove flag
	log_addr[1] = type;
	#if KEONWOO_DEBUG== 1
		printf("log_addr[1] = %d\n" , type) ; 	
	#endif 
	pos_clflush_cache_range(&log_addr[1], sizeof(unsigned long));
	#if KEONWOO_DEBUG == 1
	printf("[POS transaction start]\n\n") ; 			
	#endif
	return 1;
}

int pos_transaction_end(char *name)
{
	struct pos_name_entry *name_entry;
	unsigned long *log_addr;
	unsigned long type;
	unsigned long addr, value;
	unsigned long count;
	int index, i;
	#if KEONWOO_DEBUG == 1
	printf("[POS_transaction_end]\n") ; 	
	#endif
	name_entry = pos_lookup_name_entry(name);
	log_addr = name_entry->log_addr;

	// 1. Make the transaction flag indicate the END.
	log_addr[0] = POS_TS_END;
	#if KEONWOO_DEBUG == 1
	printf("log_addr[0] : %d\n" , POS_TS_END) ; 	
	#endif
	pos_clflush_cache_range(&log_addr[0], sizeof(unsigned long));

	// 2. In the case of removal, execute delayed pos_seg_free.
	type = log_addr[1];
	#if KEONWOO_DEBUG == 1
	printf("log_addr[1] : %d\n" , type) ; 	
	#endif
	if (type == POS_TS_REMOVE) {
		#if KEONWOO_DEBUG == 1 
		printf(" IF type POS_TS_REMOVE\n") ; 		
		#endif
		count = log_addr[MALLOC_BASE+1]; // Here, we dont' need to use pos_log_count function.
		index = MALLOC_END;
		#if KEONWOO_DEBUG == 1
		printf("count = %d , index = %d\n" , count , index) ; 	
		#endif
		for (i=0; i<count; i++) {
			addr = log_addr[index];
			value = log_addr[index+1];
			#if KEONWOO_DEBUG == 1
			printf("addr : %lu\n ", addr) ; 	
			printf("value : %d\n" , value) ; 	
			#endif
			pos_seg_free(name, (void *)addr, value);
			#if KEONWOO_DEBUG == 1 
			printf("[pos_seg_free]\n") ; 
			#endif
			index -= 2;
		}
	}

	// 3. Clear counter
	#if KEONWOO_DEBUG == 1
	printf("1) pos_log_clear_ptr \n") ; 
	#endif
	pos_log_clear_ptr(log_addr);
	#if KEONWOO_DEBUG == 1
	printf("2) pos_log_clear_free \n") ; 	
	#endif
	pos_log_clear_free(log_addr);

	// 4. Clear insert/remove flag
	log_addr[1] = 0; // type clear
	#if KEONWOO_DEBUG == 1
	printf("log_addr[1] = 0\n") ; 	
	#endif 
	pos_clflush_cache_range(&log_addr[1], sizeof(unsigned long));
	#if KEONWOO_DEBUG == 1 
	printf("[POS transaction end]\n\n") ; 
	#endif
	return 1;
}

int pos_log_insert_kv_ptr(char *name, unsigned long addr, unsigned long value)
{
	struct pos_name_entry *name_entry;
	unsigned long *log_addr;
	unsigned long count;
	int index;
	#if KEONWOO_DEBUG == 1
	printf("[pos_log_inser_kv_ptr]\n") ; 		
	#endif
	name_entry = pos_lookup_name_entry(name);
	log_addr = name_entry->log_addr;
	
	count = log_addr[KV_BASE];
	index = KV_BASE + count*2 +2;
	#if KEONWOO_DEBUG == 1
	printf(" count = %d , index = %d\n" , count , index ) ; 	
	#endif
	// 1. Insert log record.
	log_addr[index] = addr;
	log_addr[index+1] = value;
	#if KEONWOO_DEBUG == 1
	printf("[Insert Log record]\n") ; 	
	printf(" 1) log_addr[%d] = %lu\n" , index , addr) ; 	
	printf(" 2) log_addr[%d] = %ld\n" , index+1 ,value) ; 	
	#endif
	pos_clflush_cache_range(&log_addr[index], 2*sizeof(unsigned long));

	// If system failure occurs before code reaches here,
	// counter value isn't updated.
	// Thus, this log is ignored.
	
	// Also, actual value is not updated while log is being updated
	// because  actual value is updated after pos_log_insert() returns.

	// 2. Increase log counter.
	log_addr[KV_BASE] = count+1;
	pos_clflush_cache_range(&log_addr[KV_BASE], sizeof(unsigned long));
	#if KEONWOO_DEBUG == 1
	printf("[Increate Log Counter]\n") ; 	
	printf(" 1) log_addr[%d] = %d\n" , KV_BASE , count+1) ; 	
	#endif
#if LOG_CNT_ON == 1
	log_cnt++;
#endif
}

int pos_log_insert_malloc_ptr(char *name, unsigned long addr, unsigned long value)
{
	struct pos_name_entry *name_entry;
	unsigned long *log_addr;
	unsigned long count;
	int index;
	
	name_entry = pos_lookup_name_entry(name);
	log_addr = name_entry->log_addr;

	count = log_addr[MALLOC_BASE];
	index = MALLOC_BASE + count*2 +2;

	// 1. Insert log record.
	log_addr[index] = addr;
	log_addr[index+1] = value;
	pos_clflush_cache_range(&log_addr[index], 2*sizeof(unsigned long));

	// If system failure occurs before code reaches here,
	// counter value isn't updated.
	// Thus, this log is ignored.
	
	// Also, actual value is not updated while log is being updated
	// because  actual value is updated after pos_log_insert() returns.

	// 2. Increase log counter.
	log_addr[MALLOC_BASE] = count+1;
	pos_clflush_cache_range(&log_addr[MALLOC_BASE], sizeof(unsigned long));
}

int pos_log_insert_malloc_free(char *name, unsigned long addr, unsigned long value)
{
	struct pos_name_entry *name_entry;
	unsigned long *log_addr;
	unsigned long count;
	int index;
	
	name_entry = pos_lookup_name_entry(name);
	log_addr = name_entry->log_addr;

	count = log_addr[MALLOC_BASE+1];
	index = MALLOC_END - count*2;

	// 1. Insert log record.
	log_addr[index] = addr;
	log_addr[index+1] = value;
	pos_clflush_cache_range(&log_addr[index], 2*sizeof(unsigned long));

	// If system failure occurs before code reaches here,
	// counter value isn't updated.
	// Thus, this log is ignored.
	
	// Also, actual value is not updated while log is being updated
	// because  actual value is updated after pos_log_insert() returns.

	// 2. Increase log counter.
	log_addr[MALLOC_BASE+1] = count+1;
	pos_clflush_cache_range(&log_addr[MALLOC_BASE+1], sizeof(unsigned long));
}

/*int pos_log_insert_malloc(char *name, unsigned long addr, unsigned long value, int type)
{
	switch (type) {
	case POS_PTR:
		pos_log_insert_malloc_ptr(name, addr, value);
		break;
	case POS_FREE:
		pos_log_insert_malloc_free(name, addr, value);
		break;
	}

	return 1;
}*/

// #define pos_write_value(name, addr, value)				
int pos_write_value_kv(char *name, unsigned long *addr, unsigned long value)
{
	#if KEONWOO_DEBUG == 1 
	printf("  [pos_write_value_kv]\n") ; 	
	#endif
	pos_log_insert_kv_ptr(name, (unsigned long)addr, *addr);
	*addr = value;
	#if KEONWOO_DEBUG == 1 
	printf("  undo log - *addr = %ld\n" , *addr) ; 	
	#endif
	pos_clflush_cache_range((void *)addr, sizeof(unsigned long));
	#if KEONWOO_DEBUG == 1
	printf("  [pos_write_value_kv] end\n\n") ; 	
	#endif
	return 1;
}

int pos_write_value_kv_noflush(char *name, unsigned long *addr, unsigned long value)
{
	pos_log_insert_kv_ptr(name, (unsigned long)addr, *addr);
	*addr = value;
	return 1;
}

int pos_write_value_malloc(char *name, unsigned long *addr, unsigned long value)
{
	printf("[%s] %p %p\n" , addr , *addr ) ;
	pos_log_insert_malloc_ptr(name, (unsigned long)addr, *addr);
	//*addr = value;
	//pos_clflush_cache_range((void *)addr, sizeof(unsigned long));
	return 1;
}

int pos_recovery_ptr(unsigned long *log_addr)
{

	unsigned long count;
	unsigned long *addr;
	int index, i;


	// 1. KV
	count = log_addr[KV_BASE];
	index = KV_BASE + count*2;
	#if KEONWOO_DEBUG == 1 
	printf("[KV] count = %d and index = %d\n" , count , index) ; 	
	printf("[Reverse is start]\n") ; 		
	#endif 
	// Reverse order
	for (i=0; i<count; i++) {
		#if KEONWOO_DEBUG == 1 
		printf("index : %d\n" , index) ; 	
		#endif 
		addr = (unsigned long *)log_addr[index];	// target address
		#if KEONWOO_DEBUG == 1
		printf("addr recovery : %lu\n" , addr) ; 	
		#endif
		*addr = log_addr[index+1];			// target value
		#if KEONWOO_DEBUG == 1 
		printf("value recovery : %lu\n" , *addr) ; 
		printf("*addr = log_addr[%d] = %lu\n" , index+1, log_addr[index+1]) ;	
		#endif
		pos_clflush_cache_range(addr, sizeof(unsigned long));
		index -= 2;
	}
	// I don't know malloc means. 
	
	// 2. MALLOC
	count = log_addr[MALLOC_BASE];
	index = MALLOC_BASE + count*2;
	#if KEONWOO_DEBUG == 1
	printf("[MALLOC] count = %d and index = %d\n" , count , index) ; 	
	printf(" \n\n Reverse is start\n") ; 		
	#endif
	// Reverse order
	for (i=0; i<count; i++) {
		#if KEONWOO_DEBUG == 1 
		printf(" index : %d\n" , index) ; 	
		#endif
		addr = (unsigned long *)log_addr[index];	// target address
		#if KEONWOO_DEBUG == 1 
		printf("addr recovery : %lu\n" , addr) ; 
		#endif	
		*addr = log_addr[index+1];			// target value
		#if KEONWOO_DEBUG == 1 
		printf("value recovery : %d\n" , *addr) ; 	
		#endif
		pos_clflush_cache_range(addr, sizeof(unsigned long));

		index -= 2;
	}

	return 1;
}

int pos_recovery_free(char *name, unsigned long *log_addr)
	{
	unsigned long count;
	unsigned long addr, value;
	int index, i;

	count = log_addr[MALLOC_BASE+1];
	index = MALLOC_END - count*2;
	#if KEONWOO_DEBUG == 1 
	printf("[RECOVERY FREE] count : %d, index : %d\n" ,count , index) ; 		
	#endif
	// Reverse order
	for (i=0; i<count; i++) {
		#if KEONWOO_DEBUG == 1
		printf("index : %d\n" , index) ; 	
		#endif
		addr = log_addr[index];
		#if KEONWOO_DEBUG == 1
		printf("addr : %lu\n" , addr) ; 	
		#endif
		value = log_addr[index+1];
		#if KEONWOO_DEBUG == 1
		printf("value : %d\n" , value) ; 	
		printf("[pos_seg_free] name:%s,addr:%lu,value:%d\n"
			,name,addr,value) ; 	
		#endif
		pos_seg_free(name, (void *)addr, value);
		
		index += 2;
	}

	return 1;
}

int pos_recovery_insert(char *name, unsigned long *log_addr)
{
	#if KEONWOO_DEBUG == 1
	printf("\n******POS_recovery_insert******\n") ; 
	#endif	
	struct pos_name_entry *name_entry;
	int se_type; // START or END
	se_type = log_addr[0];
		
	if (se_type == POS_TS_END) {
		// Transaction ended normally.
		// So, we can do nothing.
		#if KEONWOO_DEBUG == 1
		printf("transaction status END\n") ; 	
		#endif
		return 1;
	} else if (se_type == POS_TS_START) {
		#if KEONWOO_DEBUG == 1 
		printf("transaction status START not END yet\n") ; 
		#endif
		// Transaction ended abnormally.
		// Execute undo both PTR and FREE.
		#if KEONWOO_DEBUG == 1 
		printf("[pos_recovery_ptr]\n") ; 	
		#endif 
		pos_recovery_ptr(log_addr);
		#if KEONWOO_DEBUG == 1 
		printf("[pos_log_clear_ptr]\n") ; 	
		#endif 
		pos_log_clear_ptr(log_addr);
		#if KEONWOO_DEBUG == 1
		printf("[pos_recovery_free]\n") ; 	
		#endif 
		pos_recovery_free(name, log_addr);
		#if KEONWOO_DEBUG == 1 
		printf("[pos_log_clear_free]\n") ;	
		#endif
		pos_log_clear_free(log_addr);
	}
	return 1;
}

int pos_recovery_remove(unsigned long *log_addr)
{
	int se_type; // START or END

	se_type = log_addr[0];

	if (se_type == POS_TS_START) {
		// Transaction ended abnormally.
		// Execute undo PTR.
		pos_recovery_ptr(log_addr);
		pos_log_clear_ptr(log_addr);
	} else if (se_type == POS_TS_END) {
		// Transaction ended normally.
		// But, pos_seg_free may not be excuted normally.
		// So. execute redo FREE.
		pos_recovery_ptr(log_addr);
		pos_log_clear_ptr(log_addr);
	}
	
	return 1;
}

int pos_recovery(char *name)
{
	#if KEONWOO_DEBUG == 1
	printf("\n\n\n ======POS RECOVERY===== \n") ; 		
	#endif
	struct pos_name_entry *name_entry;
	unsigned long *log_addr;
	unsigned long type;

	name_entry = pos_lookup_name_entry(name);
	log_addr = name_entry->log_addr;
	#if KEONWOO_DEBUG == 1 
	printf("Log addr : %lu\n" , log_addr ) ; 	
	#endif

	type = log_addr[1];
	#if KEONWOO_DEBUG == 1 
	if( type == 0x12345678 ) 
	printf("Your status during execute insert\n") ; 	
	else if(type == 0x87654321 )
	printf("Your status during execute remove\n") ; 	
	else printf("Your status = %lu\n" , type) ; 	
	#endif

	switch (type) {
	case POS_TS_INSERT:
		return pos_recovery_insert(name, log_addr);
	case POS_TS_REMOVE:
		return pos_recovery_remove(log_addr);
	default:
		// Transaction was about to start or ended normally.
		// But, power failure occured while type is being updated.
		// So, in this case, we can do nothing.
		log_addr[1] = 0;
		pos_clflush_cache_range(&log_addr[1], sizeof(unsigned long));
		return 1;
	}
	#if KEONWOO_DEBUG == 1
	printf("========RECOVER END=====\n") ; 
	#endif
}

#define mb() 	asm volatile("mfence":::"memory")

static inline void clflush(volatile void *__p)
{
	//asm volatile("clflush %0" : "+m" (*(volatile char __force *)__p));
	asm volatile("clflush %0" : "+m" (*(volatile char *)__p));

#if LOG_CNT_ON == 1
	clflush_cnt++;
#endif
}

int x86_64_clflush_size = 64;

/**
 * clflush_cache_range - flush a cache range with clflush
 * @addr:	virtual start address
 * @size:	number of bytes to flush
 *
 * clflush is an unordered instruction which needs fencing with mfence
 * to avoid ordering issues.
 */
static int clflush_count = 0 ; 	
pthread_mutex_t a_mutex = PTHREAD_MUTEX_INITIALIZER;
void a_lock(){ pthread_mutex_lock(&a_mutex);}
void a_unlock(){ pthread_mutex_unlock(&a_mutex);}
void get_clflush_count(){ 
	printf("%d\n" , clflush_count) ;
}

void pos_clflush_cache_array(void **arr , int flush_cnt){
        /* only for arm clflush */
//      syscall(393, arr , flush_cnt) ;         
//      clflush_count++ ;       
//      return ;        

        /* only for x86 clflush */
        void *end ;
        int i = 0 ;
        mb() ;
        for( i = 0 ; i < flush_cnt; i++){
                clflush(arr[i]) ;
        }
        mb() ;
        return ;
}

 
void pos_clflush_cache_range(void *vaddr, unsigned int size)
{
//	return ; // for test...
//	a_lock() ; 	
//	clflush_count++ ; 	
///	a_unlock() ; 	
//	printf("[%d]-clflush_count\n", 	clflush_count) ; 	
	


	void *vend = vaddr + size - 1;

	mb();

	for (; vaddr < vend; vaddr += x86_64_clflush_size)
		clflush(vaddr);
	/*
	 * Flush any possible final partial cacheline:
	 */
	clflush(vend);

	mb();
}
