#include "contiki.h"
#include "CuTest.h"
#include "cfs/cfs.h"
#include <stdio.h>
#include <string.h>
#include "btstack/src/remote_device_db.h"
void test_cfs(CuTest* tc)
{
  cfs_coffee_format();

   /*        */
  /* step 1 */
  /*        */
  char message[32];
  char buf[100];
  strcpy(message,"#1.hello world.");
  strcpy(buf,message);

  /* End Step 1. We will add more code below this comment later */    
  /*        */
  /* step 2 */
  /*        */
  /* writing to cfs */
  const char *filename = "/sys/msg_file";
  int fd_write, fd_read;
  int n;
  fd_write = cfs_open(filename, CFS_WRITE);
  if(fd_write != -1) {
    n = cfs_write(fd_write, message, sizeof(message));
    cfs_close(fd_write);
    //printf("step 2: successfully written to cfs. wrote %i bytes\n", n);
  } else {
    CuFail(tc, "ERROR: could not write to memory in step 2.\n");
  } 
  /*        */
  /* step 3 */
  /*        */
  /* reading from cfs */
  strcpy(buf,"empty string");
  fd_read = cfs_open(filename, CFS_READ);
  if(fd_read!=-1) {
    cfs_read(fd_read, buf, sizeof(message));
    printf("step 3: %s\n", buf);
    cfs_close(fd_read);
  } else {
    CuFail(tc, "ERROR: could not read from memory in step 3.\n");
  }
  /*        */
  /* step 4 */
  /*        */
  /* adding more data to cfs */
  strcpy(buf,"empty string");
  strcpy(message,"#2.contiki is amazing!");
  fd_write = cfs_open(filename, CFS_WRITE | CFS_APPEND);
  if(fd_write != -1) {
    n = cfs_write(fd_write, message, sizeof(message));
    cfs_close(fd_write);
    printf("step 4: successfully appended data to cfs. wrote %i bytes  \n",n);
  } else {
    CuFail(tc, "ERROR: could not write to memory in step 4.\n");
  }
  /*        */
  /* step 5 */
  /*        */
  /* seeking specific data from cfs */
  strcpy(buf,"empty string");
  fd_read = cfs_open(filename, CFS_READ);
  if(fd_read != -1) {
    cfs_read(fd_read, buf, sizeof(message));
    CuAssertTrue(tc, strcmp(buf, "#1.hello world.") == 0);
    printf("step 5: #1 - %s\n", buf);
    cfs_seek(fd_read, sizeof(message), CFS_SEEK_SET);
    cfs_read(fd_read, buf, sizeof(message));
    CuAssertTrue(tc, strcmp(buf, "#2.contiki is amazing!") == 0);
    cfs_close(fd_read);
  } else {
    CuFail(tc, "ERROR: could not read from memory in step 5.\n");
  }
  /*        */
  /* step 6 */
  /*        */
  /* remove the file from cfs */
  cfs_remove(filename);
  fd_read = cfs_open(filename, CFS_READ);
  if(fd_read == -1) {
    printf("Successfully removed file\n");
  } else {
    CuFail(tc, "ERROR: could read from memory in step 6.\n");
  }

  // COPY A SCRIPT FILE FOR SCRIPT TESTING
  FILE* fp = fopen("script1.amx", "rb");
  int fd = cfs_open("/script1.amx", CFS_WRITE);

  if (fp != NULL && fd != -1)
  {
    // copy the file
    char buf[1024];
    int length;

    length = fread(buf, 1, 1024, fp);
    while(length > 0)
    {
      printf("write %d bytes\n", length);
      cfs_write(fd, buf, length);
      length = fread(buf, 1, 1024, fp);
    }
    fclose(fp);
    cfs_close(fd);
  }
}

void test_remote_db(CuTest* tc)
{
  bd_addr_t bd = {1, 2, 3, 4, 5, 6};
  link_key_t key = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  CuAssertIntEquals(tc, remote_device_db_memory.get_link_key(&bd, &key), 0);

  remote_device_db_memory.put_link_key(&bd, &key);

  CuAssertIntEquals(tc, remote_device_db_memory.get_link_key(&bd, &key), 1);

  // validate key
  for(int i = 0; i < 16; i++)
  {
    printf("%d ", key[i]);
    CuAssertTrue(tc, key[i] == i+1);
  }
  printf("\n");

  bd[0] = 0xff;
  key[0] = 0xff;
  remote_device_db_memory.put_link_key(&bd, &key);

  CuAssertIntEquals(tc, remote_device_db_memory.get_link_key(&bd, &key) , 1);

  // validate key
  CuAssertIntEquals(tc, key[0], 0xff);
  for(int i = 1; i < 16; i++)
    CuAssertIntEquals(tc, key[i], i+1);

  remote_device_db_memory.delete_link_key(&bd);

  CuAssertIntEquals(tc, remote_device_db_memory.get_link_key(&bd, &key) ,  0);
}

CuSuite* cfsGetSuite(void)
{
	CuSuite* suite = CuSuiteNew();

	SUITE_ADD_TEST(suite, test_cfs);
	SUITE_ADD_TEST(suite, test_remote_db);


}