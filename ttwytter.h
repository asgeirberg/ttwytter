#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <oauth.h>
#include <curl/curl.h>
#include <stdarg.h>
#include "cJSON.h"

#define FORMAT_SIZE 5000

int init_libcurl(int argc,  char **argv); //this just initilizes some things needed for libcurl
int get_data(char *screen_name);
int parse_jason(char *response); //parses the output from libcurl.
int read_from_file(char *filename, FILE *f); /*reads input from file when -f is used. stdin is passed as the file pointer when -f is not used  */
void output(FILE *stream, const char *output, ...);
size_t static write_to_memory(void *response, size_t size, size_t nmemb, void *userp);
int parse_arguments(int argc, char **argv);

char* consumer_key = NULL;
char* consumer_secret = NULL;
char* user_token = NULL;
char* user_secret = NULL;

/* command line flags and arguments */
unsigned short int user_flag = 0;
unsigned short int post_tweet_flag = 0;
unsigned short int quiet_flag = 0; /* silences errors to the terminal. That which is sent to stdout still is output. */
unsigned short int time_flag = 0;
unsigned short int file_flag = 0;
unsigned short int supress_output_flag = 0; /* supresses all output in getting tweets except  */
char *filename;

/* this is used by write_to_memory() to reserve memory for the response. */
struct Buffer {
  char *memory;
  size_t size;
};

struct Buffer response;

/* function arguments after parsing */
char *screen_name;
char *count;

//CURL *curl;

FILE *f = NULL;

int init_libcurl(int argc, char **argv)
{
  consumer_key = "scgvVzQDrwKfbZUZtepaZOvdz";
  consumer_secret = "Ou0NfStxJxSjR0y1i0GuZHQUBQRd3e7yQ8LQrd0WROSrxTfhtc";
  user_token = "1128798817-TdkOFKyFFuhUmu1SQNu58dokhm3ZdrTSHoYfcUq";
  user_secret = "XQCTY1aro7vx3Hb14SLgLewYoiSyofDxqRFAlZAQou2qa";

  return 0;
}

int get_data(char *screen_name)
{
  response.memory = malloc(1);  /* will be grown as needed by using realloc */ 
  response.size = 0;    /* no data at this point */

  CURL *curl = curl_easy_init();

  char *url = malloc(sizeof(char) * 256);

  strcpy(url, "https://api.twitter.com/1.1/statuses/user_timeline.json");
  strcat(url, "?screen_name=");
  strcat(url, screen_name);
  strcat(url, "&count=");
    
  if (count == NULL) /* if count is not set by the user, 1 is put as default. */
  {
    count = malloc(sizeof(char) * 2);
    strcpy(count, "1");
  }

  strcat(url, count);

  char *signedurl = oauth_sign_url2(url, NULL, OA_HMAC, "GET", consumer_key, consumer_secret, user_token, user_secret);

  curl_easy_setopt(curl, CURLOPT_URL, signedurl); /* URL we're connecting to, after being signed by oauthlib */
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "ttwytter/0.1"); /* User agent we're going to use */
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1); /* libcurl will now fail on an HTTP error (>=400) */
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_memory); /* setting a callback function to return the data */
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response); /* passing the pointer to the response as the callback parameter */
    
  int curlstatus = curl_easy_perform(curl); /* Execute the request! */

  if (curlstatus != 0) /* this might be rewritten with a case structure, to better distinguish errors */
    {
      if (curlstatus == 22)
      {
        output(stderr, "No such user as %s", screen_name); 
      }
      else 
      {
        output(stderr, "curl_easy_perform terminated with status code %d\n", curlstatus); 
      }  
      
      return 1;
    }
  else
    {
      parse_jason(response.memory);  
    }

  curl_easy_cleanup(curl);
  free (signedurl);
  free(url);

  return 0;
}

int parse_jason(char *response)
{

  cJSON *array, *root, *child;
  char *text, *screen_name, *time_stamp;  

  root = cJSON_Parse(response);

  // char *test = cJSON_Print(root); */ this is here for debugging reasons */
  // printf("%s\n", test);

  if (!root) 
  {
    printf("Error before: [%s]\n",cJSON_GetErrorPtr());
    return 1;
  }
  else
  {
    for (int i = 0; i < cJSON_GetArraySize(root); ++i)
    {
      array = cJSON_GetArrayItem(root, i); 
      
      if (!array)
      {
        printf("Error before: [%s]\n",cJSON_GetErrorPtr());
        
        return 1;
      }

      //actual parsing
      text = cJSON_GetObjectItem(array, "text")->valuestring; /* find the key we want */

      if (time_flag)
      {
          time_stamp = cJSON_GetObjectItem(array, "created_at")->valuestring;
      }

      child = cJSON_GetObjectItem(array, "user");
      screen_name = cJSON_GetObjectItem(child, "screen_name")->valuestring;

      output(stdout, "\"%s\" ", text); /* By doing this seperately, we have more control over the output. TODO: Put the output in it's own routine */
      
      if (!supress_output_flag)
      {   
        output(stdout, "by %s", screen_name);
      }

      if (time_flag && !supress_output_flag)
      {
          output(stdout, " at %s", time_stamp);
      }

      output(stdout, "\n");
      
    }

    cJSON_Delete(root); /* dereference the pointer */

    return 0; 
    } 
}

int read_from_file(char *filename, FILE *f) /* reads input from file when -f is used or stdin when no argument is given*/
{
  size_t len = 0;
  ssize_t read;
  screen_name = malloc(sizeof(char) * 16);

  if (filename != NULL) /* if filename is NULL, stdin is being used, and thus no need to open the file*/
  {
    f = fopen(filename, "r");
    if (f == NULL)
    {
      printf("Error opening file %s.\n", filename);

      return 1;
    }
  } 

  while ((read = getline(&screen_name, &len, f)) != -1)
  {
    if (strlen(screen_name) > 16 )
    {
      output(stderr, "Error: Username must be less than 16 characters. Use -q to suppress this warning.\n", screen_name);
      fflush (f);
    }
    else 
    {
      get_data(screen_name);
    }
  }
  
  free(screen_name);
  fclose(f);

  return 0;
}

void output(FILE *stream, const char *format, ...) /* outputs text. It's written in this way to allow the q-flag in a simple way. */
{
    char msg[FORMAT_SIZE];
    va_list ap;

    va_start(ap, format);
    vsnprintf(msg, sizeof(msg) - sizeof(char), format, ap);

    if ((!quiet_flag && stream == stderr) || stream == stdout)
    {
      fprintf(stream, "%s", msg);
      fflush(stream); 
    }
}

static size_t write_to_memory(void *response, size_t size, size_t nmemb, void *userp) /* this is basically from libcurl's getinmemory.c example. No reason to re-invent the wheel. */
{
  size_t realsize = size * nmemb;
  struct Buffer *mem = (struct Buffer *)userp;
 
  mem->memory = realloc(mem->memory, mem->size + realsize + 1);

  if (mem->memory == NULL)
  {
    /* out of memory! */ 
    printf("Not enough memory (realloc returned NULL)\n");

    return 0;
  }
 
  memcpy(&(mem->memory[mem->size]), response, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}