#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <oauth.h>
#include <curl/curl.h>
#include <stdarg.h>
#include <jansson.h>

#define FORMAT_SIZE 5000

char* consumer_key = NULL;
char* consumer_secret = NULL;
char* user_token = NULL;
char* user_secret = NULL;

/* command line flags and arguments */
unsigned short int alert_flag = 0; 
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

struct Output {
  const char *text;
  const char *screen_name;
  const char *time_stamp; 
};

typedef struct Output Data;

/* function arguments after parsing */
char *screen_name;
char *count;

//CURL *curl;

FILE *f = NULL;

int init_libcurl(int argc,  char **argv); //this just initilizes some things needed for libcurl
char *get_data(char *screen_name);
Data *parse_json(char *response); //parses the output from libcurl.
int read_from_file(char *filename, FILE *f); /*reads input from file when -f is used. stdin is passed as the file pointer when -f is not used  */
void output(FILE *stream, const char *output, ...);
size_t static write_to_memory(void *response, size_t size, size_t nmemb, void *userp);
int parse_arguments(int argc, char **argv);


int init_libcurl(int argc, char **argv)
{
  consumer_key = "scgvVzQDrwKfbZUZtepaZOvdz";
  consumer_secret = "Ou0NfStxJxSjR0y1i0GuZHQUBQRd3e7yQ8LQrd0WROSrxTfhtc";
  user_token = "1128798817-TdkOFKyFFuhUmu1SQNu58dokhm3ZdrTSHoYfcUq";
  user_secret = "XQCTY1aro7vx3Hb14SLgLewYoiSyofDxqRFAlZAQou2qa";

  return 0;
}

char *get_data(char *screen_name)
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
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "ttwytter/0.2"); /* User agent we're going to use */
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
      return NULL;
    }

  curl_easy_cleanup(curl);
  free (signedurl);
  free(url);

  return response.memory;
}

Data *parse_json(char *response)
{
  
  char *end;
  int i_count = strtol(count, NULL, 10);
  int i;

  json_t *root;
  json_error_t error;

  /* allocate memory for the output */
  Data *parsed_struct = NULL;
  parsed_struct = malloc(i_count * sizeof(Data));
      
  root = json_loads(response, 0, &error);

  if (!root)
  {
    fprintf(stderr, "Error: on line %d: %s\n", error.line, error.text);
    
    return NULL;
  }

  if (!json_is_array(root))
  {
    fprintf(stderr, "Error: root is not an array\n");
    json_decref(root);

    return NULL;
  }

  for(int i = 0; i < json_array_size(root); i++)
  {
    json_t *data, *text_object, *time_stamp_object, *user_object, *screen_name_object;

    data = json_array_get(root, i);

    if (!json_is_object(data))
    {
        fprintf(stderr, "Error: commit data %d is not an object\n", i + 1);
        json_decref(root);

        return NULL;
    }

    text_object = json_object_get(data, "text");
    parsed_struct[i].text = strdup(json_string_value(text_object));
    

    if (time_flag)
    {
    time_stamp_object = json_object_get(data, "created_at");
    parsed_struct[i].time_stamp = strdup(json_string_value(time_stamp_object));
        
    }

    user_object = json_object_get(data, "user");
    screen_name_object = json_object_get(user_object, "screen_name");
    parsed_struct[i].screen_name = strdup(json_string_value(screen_name_object));
  
  }

json_decref(root);

return parsed_struct; 
}

void output_data(Data *parsed_struct) /* Segfaults if -c is higher than the tweets actually received */
{
  char *end;
  int i_count = strtol(count, NULL, 10);
  int i;

  for (i = 0; i < i_count; ++i)
  {
    output(stdout, "\"%s\" ", parsed_struct[i].text);
      
    if (!supress_output_flag)
    {   
      output(stdout, "by %s", parsed_struct[i].screen_name);
    }

    if (time_flag && !supress_output_flag)
    {
      output(stdout, " at %s", parsed_struct[i].time_stamp);
    }

    output(stdout, "\n");

    if (alert_flag)
    {
      output(stderr, "\a");
    }
  }
}

int read_from_file(char *filename, FILE *f) /* reads input from file when -f is used or stdin when no argument is given*/
{
  size_t len = 0;
  ssize_t read;
  screen_name = malloc(sizeof(char) * 16);

  char *response_string;
  Data *parsed_struct = NULL;

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
      response_string = get_data(screen_name); /* get the tweet with the username set with -u*/
      parsed_struct = parse_json(response_string);
      output_data(parsed_struct);
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

/* Below is unused code */

// int stream()
// {
// char* response_string = NULL;
// char* first_response_string = NULL;

// /* Set up the select() function */
// fd_set input_set;
// struct timeval timeout;
// int ready_for_reading = 0;

// /* Waiting for some seconds */
// timeout.tv_sec = 5;    // 30 seconds is the default
// timeout.tv_usec = 0;    // 0 milliseconds

// char input;
// int len;

// printf("Listening to stream @%s. Use 'ctrl-D' to send EOF and break the stream.\n", screen_name);
// first_response_string = get_data(screen_name); /* get the last tweet to compare with what we get to the stream. We only print new tweets. */

//    while (input != EOF)
//    {
//         /* Selection */
//         FD_ZERO(&input_set );   /* Empty the FD Set */
//         FD_SET(0, &input_set);  /* Listen to the input descriptor */
//         int ready_for_reading = select(1, &input_set, NULL, NULL, &timeout);

//         /* Selection handling */
//         if (ready_for_reading < 0)
//         {
//             printf("select() returned an error.\n");

//             return 1;
//         }
//         else if (ready_for_reading)
//         {
//             input = fgetc(stdin);  
//         }

//         if (!input)
//         {
//           response_string = get_data(screen_name); /* get the tweet with the username set with -u*/
//           strlcpy(first_response_string, response_string, sizeof(response_string));
        
//           printf(".\n");

//           if (strcmp(first_response_string, response_string) == 0)
//           {
//             parsed_struct = parse_json(response_string);
//             printf("%s\n", parsed_struct[0].text);
//             //output_data(parsed_struct);
//           }
//         }     
//    }

//    return 0;
// }

// int get_feed(char *screen_name) /* Parsing doesn't work. -e switches this on.*/ 
// {
//   response.memory = malloc(1);  /* will be grown as needed by using realloc */ 
//   response.size = 0;    /* no data at this point */

//   CURL *curl = curl_easy_init();

//   char *url = malloc(sizeof(char) * 256);

//   strcpy(url, "https://userstream.twitter.com/1.1/user.json");

//   char *signedurl = oauth_sign_url2(url, NULL, OA_HMAC, "GET", consumer_key, consumer_secret, user_token, user_secret);

//   curl_easy_setopt(curl, CURLOPT_URL, signedurl); /* URL we're connecting to, after being signed by oauthlib */
//   curl_easy_setopt(curl, CURLOPT_USERAGENT, "ttwytter/0.2"); /* User agent we're going to use */
//   curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1); /* libcurl will now fail on an HTTP error (>=400) */
//   //curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_memory); /* setting a callback function to return the data */
//   //curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response); /* passing the pointer to the response as the callback parameter */
//   curl_easy_setopt(curl, CURLOPT_WRITEDATA, stdout); /* passing the pointer to the response as the callback parameter */
    
//   int curlstatus = curl_easy_perform(curl); /* Execute the request! */

//   if (curlstatus != 0) /* this might be rewritten with a case structure, to better distinguish errors */
//     {
//       if (curlstatus == 22)
//       {
//         output(stderr, "No such user as %s", screen_name); 
//       }
//       else 
//       {
//         output(stderr, "curl_easy_perform terminated with status code %d\n", curlstatus); 
//       }  
      
//       return 1;
//     }
//   else
//     {
//       parse_jason(response.memory);  
//     }

//   curl_easy_cleanup(curl);
//   free (signedurl);
//   free(url);

//   return 0;
// }


