#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <oauth.h>
#include <curl/curl.h>
#include <stdarg.h>
#include <jansson.h>

#define FORMAT_SIZE 5000
#define TWEET_SIZE 512

/* Paths to the appropriate Twitter APIs.*/ 
#define USER_TIMELINE "https://api.twitter.com/1.1/statuses/user_timeline.json" /* gets last n tweets from a given user*/
#define MENTIONS_TIMELINE "https://api.twitter.com/1.1/statuses/mentions_timeline.json" /* gets the last n mentions of the authenticated user*/
#define HOME_TIMELINE "https://api.twitter.com/1.1/statuses/home_timeline.json" /* gets the last n tweets from the authenticated user's timeline*/
#define POST_TWEET "https://api.twitter.com/1.1/statuses/update.json" /* posts tweets */
#define DESTROY_TWEET "https://api.twitter.com/1.1/statuses/destroy/" /* destroy tweets */

char* consumer_key = NULL;
char* consumer_secret = NULL;
char* user_token = NULL;
char* user_secret = NULL;

/* command line flags and arguments */

unsigned short int alert_flag = 0; /* plays alerts */
unsigned short int user_flag = 0;
unsigned short int post_tweet_flag = 0;
unsigned short int destroy_tweet_flag = 0;
unsigned short int quiet_flag = 0; /* silences errors to the terminal. That which is sent to stdout still is output. */
unsigned short int time_flag = 0;
unsigned short int file_flag = 0;
unsigned short int supress_output_flag = 0; /* supresses all output in getting tweets except  */
unsigned short int stream_flag = 0;
unsigned short int mentions_flag = 0; /* Gets the last n mentions of the authenticated user */
unsigned short int timeline_flag = 0; /* Gets the last n items in the authenticated user's timeline */
unsigned short int verbose_flag = 0;
unsigned short int id_flag = 0;
char *filename;
char *postdata;

/* this is used by write_to_memory() to reserve memory for the response. */
struct Buffer {
  char *memory;
  size_t size;
};

struct Buffer response;

struct Output {
  int size; /* This is the size of the struct itself, used to keep track of the output in ttwytter_output_data(). It is set in ttwytter_parse_json()*/ 
  const char *text;
  const char *screen_name;
  const char *time_stamp; 
  const char *id_str;
};

typedef struct Output Data;

/* function arguments after parsing */
char *screen_name;
char *count;

FILE *f = NULL;

int ttwytter_init_libcurl(int argc,  char **argv); //this just initilizes some things needed for libcurl
char *ttwytter_build_url(char *screen_name);
char *ttwytter_build_header(char *url, char *url_enc_args);
char *ttwytter_request(char *http_method, char *url, char *url_enc_args);
char *ttwytter_get_data(char *screen_name);
int ttwytter_post_data(char *tweet);
Data *ttwytter_parse_json(char *response); //parses the output from libcurl.
int twytter_check_curl_status(int curlstatus);
int ttwytter_read_from_file(char *filename, FILE *f); /*reads input from file when -f is used. stdin is passed as the file pointer when -f is not used  */
void ttwytter_output(FILE *stream, const char *output, ...);
size_t static write_to_memory(void *response, size_t size, size_t nmemb, void *userp);
char *remove_first_char(char* string);
int parse_arguments(int argc, char **argv);


int ttwytter_init_libcurl(int argc, char **argv)
{
  consumer_key = "ztmB7Bda6O3EJcz1XYEC3NILX";
  consumer_secret = "PsF8UXmCit7vReiLmWI2oP8uzyK9yJd527aGwPuWsf46dG3SJJ";
  user_token = "1128798817-Z5EnXaAHX3dEs95tcVCXuohkIkv2Yxm6ELj8qTs";
  user_secret = "liR9ZXq2c5OFIa2poMdyiGMUIX3jr3yPJDRWLGoCEcyHT";

  return 0;
}

char *ttwytter_build_url(char *screen_name)
{
  char *url = malloc(sizeof(char) * 256);

  if (timeline_flag)
  {
    strcpy(url, HOME_TIMELINE);
    strcat(url, "&count=");
    
    if (count == NULL) /* if count is not set by the user, 5 is put as default. */
    {
      count = malloc(sizeof(char) * 2);
      strcpy(count, "5");
    }

    strcat(url, count);
  }
  else if (destroy_tweet_flag)
  {

  }
  else if (mentions_flag)
  {
    strcpy(url, MENTIONS_TIMELINE);
    strcat(url, "&count=");
    
    if (count == NULL) /* if count is not set by the user, 5 is put as default. */
    {
      count = malloc(sizeof(char) * 2);
      strcpy(count, "5");
    }

    strcat(url, count);
  }
  else
  {
    strcpy(url, USER_TIMELINE);
    strcat(url, "?screen_name=");
    strcat(url, screen_name);
    strcat(url, "&count=");
    
    if (count == NULL) /* if count is not set by the user, 1 is put as default. */
    {
      count = malloc(sizeof(char) * 2);
      strcpy(count, "1");
    }

    strcat(url, count);
  }

  return url;
}

// char* ttwytter_build_header(char *url, char *url_enc_args)
// {
//   char *postdata = NULL;
//   struct curl_slist *slist = NULL;
//   char * ser_url, **argv, *auth_params, *auth_header, *non_auth_params, *final_url, *temp_url;
//   int argc;

//   auth_header = malloc(1024);

//   if (url_enc_args == NULL)  /* concatenate the url and any url-encoded arguments if*/ 
//   {
//     ser_url = malloc(strlen(url) + 1);
//     strcpy(ser_url, url);
//   }
//   else 
//   {
//     ser_url = malloc(strlen(url) + strlen(url_enc_args) + 2); /* This is the general behaviour*/
//     sprintf(ser_url, "%s?%s", url, url_enc_args);  
//   }

//   argv = malloc(0); /* oauth_split_url_parameters builds an array of URL encoded parameters from ser_url and stores it in argv  */ 
//   argc = oauth_split_url_parameters(ser_url, &argv);
//   free(ser_url);

//   temp_url = oauth_sign_array2(&argc, &argv, NULL, OA_HMAC, "POST", consumer_key, consumer_secret, user_token, user_secret); /* Here we sign the array*/
//   free(temp_url);

//   auth_params = oauth_serialize_url_sep(argc, 1, argv, ", ", 6); /* Here we finally build the oauth header*/
//   sprintf(auth_header, "Authorization: OAuth %s", auth_params );

//   free(auth_params);

//   return auth_header;
// }

char *ttwytter_get_data(char *screen_name)
{
  if (user_flag)
  {
    return ttwytter_request("GET", USER_TIMELINE, screen_name);
  }
  else if (mentions_flag)
  {
    return ttwytter_request("GET", MENTIONS_TIMELINE, screen_name);
  }
  else if (timeline_flag)
  {
    return ttwytter_request("GET", HOME_TIMELINE, NULL);
  }

  return NULL;
}

int ttwytter_post_data(char *postdata)
{
  char *url_enc_args = NULL;

  if (post_tweet_flag)
  {
    url_enc_args = malloc(sizeof(char) * 512);

    sprintf(url_enc_args, "status=%s", postdata);
    ttwytter_request("POST", POST_TWEET, url_enc_args);

    free(url_enc_args);
  }

  if (destroy_tweet_flag)
  {
    url_enc_args = malloc(sizeof(char) * 512);

    sprintf(url_enc_args, "%s.json", postdata);
    ttwytter_request("POST", DESTROY_TWEET, url_enc_args); /* destroying tweets appends id.json to the url*/

    free(url_enc_args);
  }

  return 0;
}

char *ttwytter_request(char *http_method, char *url, char *url_enc_args) /* TODO: rewrite this to a more modular design, too much code is being reused */
{

  CURL *curl = curl_easy_init();

  if (!strcmp(http_method, "GET"))
  {
    response.memory = malloc(1);  /* will be grown as needed by using realloc */ 
    response.size = 0;    /* no data at this point */

    char *url = ttwytter_build_url(url_enc_args); 
    char *signedurl = oauth_sign_url2(url, NULL, OA_HMAC, "GET", consumer_key, consumer_secret, user_token, user_secret);

    curl_easy_setopt(curl, CURLOPT_URL, signedurl); /* URL we're connecting to, after being signed by oauthlib */
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ttwytter/0.4"); /* User agent we're going to use */
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1); /* libcurl will now fail on an HTTP error (>=400) */

    if (verbose_flag)
    {
      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    }

    free(url);
  }
  else if (!strcmp(http_method, "POST")) /* The code to build the header should be in its own function.*/
  {
    if (post_tweet_flag)
    {
      struct curl_slist * slist = NULL;
      char * ser_url, **argv, *auth_params, auth_header[1024], *non_auth_params, *final_url, *temp_url;
      int argc;

      if (url_enc_args == NULL)
      {
        ser_url = (char *) malloc(strlen(url) + 1);
        strcpy(ser_url, url);
      }
      else
      {
        ser_url = (char *) malloc(strlen(url) + strlen(url_enc_args) + 2);
        sprintf(ser_url, "%s?%s", url, url_enc_args);
      }

      argv = malloc(0);
      argc = oauth_split_url_parameters(ser_url, &argv);
      free(ser_url);

      temp_url = oauth_sign_array2(&argc, &argv, NULL, OA_HMAC, http_method, consumer_key, consumer_secret, user_token, user_secret);
      free(temp_url);

      auth_params = oauth_serialize_url_sep(argc, 1, argv, ", ", 6);
      sprintf( auth_header, "Authorization: OAuth %s", auth_params );
      slist = curl_slist_append(slist, auth_header);
      free(auth_params);

      non_auth_params = oauth_serialize_url_sep(argc, 1, argv, "", 1 );

      final_url = (char *) malloc( strlen(url) + strlen(non_auth_params) );

      strcpy(final_url, url);
      postdata = non_auth_params;

      for (int i = 0; i < argc; i++ )
      {
        free(argv[i]);   
      }
      free(argv);

      curl_easy_setopt(curl, CURLOPT_URL, url);
      curl_easy_setopt(curl, CURLOPT_USERAGENT, "ttwytter/0.4"); /* User agent we're going to use */
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
      curl_easy_setopt(curl, CURLOPT_POST, 1 );
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata);

      if (verbose_flag)
      {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
      }
    }  
  }
  else
  {
    ttwytter_output(stderr, "Invalid http-method.\n");
  }
  
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_memory); /* setting a callback function to return the data */
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response); /* passing the pointer to the response as the callback parameter */ 

  int curlstatus = curl_easy_perform(curl); /* Execute the request! */

  curl_easy_cleanup(curl);

  if (twytter_check_curl_status(curlstatus)) /* Handles error messages from cURL */
  {
    return NULL;
  } 
  else
  {
    return response.memory;  
  }
}

Data *ttwytter_parse_json(char *response)
{
  json_t *root;
  json_error_t error;

  /* allocate memory for the output */
  root = json_loads(response, 0, &error);
  Data *parsed_struct = NULL;
  parsed_struct = malloc(json_array_size(root) * sizeof(Data));

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
    json_t *data, *text_object, *time_stamp_object, *user_object, *screen_name_object, *id_str_object; 

    data = json_array_get(root, i);

    if (!json_is_object(data))
    {
        fprintf(stderr, "Error: commit data %d is not an object\n", i + 1);
        json_decref(root);

        return NULL;
    }

    text_object = json_object_get(data, "text");
    parsed_struct[i].text = strdup(json_string_value(text_object));
    
    id_str_object = json_object_get(data, "id_str");
    parsed_struct[i].id_str = strdup(json_string_value(id_str_object));

    if (time_flag)
    {
      time_stamp_object = json_object_get(data, "created_at");
      parsed_struct[i].time_stamp = strdup(json_string_value(time_stamp_object));
    }

    user_object = json_object_get(data, "user");
    screen_name_object = json_object_get(user_object, "screen_name");
    parsed_struct[i].screen_name = strdup(json_string_value(screen_name_object));
  
  }

parsed_struct[0].size = json_array_size(root); /* used in ttwytter_output_data to keep track of how much we received here. */
json_decref(root);

return parsed_struct; 
}

int twytter_check_curl_status(int curlstatus)
{
  if (curlstatus != 0) /* this might be rewritten with a case structure, to better distinguish errors */
    {
      if (curlstatus == 22)
      {
        ttwytter_output(stderr, "No such user as %s.\n", screen_name); 
      }
      else 
      {
        ttwytter_output(stderr, "curl_easy_perform terminated with status code %d\n", curlstatus); 
      }  
      return 1;
    }
  return 0;  
}

void ttwytter_output_data(Data *parsed_struct)
{
  for (int i = 0; i < parsed_struct[0].size; ++i)
  {
    if (1) /* Here we will put a real flag to supress text too. */
    {  
      ttwytter_output(stdout, "\"%s\" ", parsed_struct[i].text);
    }  

    if (!supress_output_flag)
    {   
      ttwytter_output(stdout, "by %s", parsed_struct[i].screen_name);
    }

    if (time_flag && !supress_output_flag)
    {
      ttwytter_output(stdout, " at %s", parsed_struct[i].time_stamp);
    }

    if (id_flag && !supress_output_flag)
    {
      ttwytter_output(stdout, " %s", parsed_struct[i].id_str); /* TODO: make this look nicer in the output */
    }

    ttwytter_output(stdout, "\n");

    if (alert_flag)
    {
      ttwytter_output(stderr, "\a");
    }
  }
}

int ttwytter_read_from_file(char *filename, FILE *f) /* reads input from file when -f is used or stdin when no argument is given*/
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
      ttwytter_output(stderr, "Error opening file %s.\n", filename);

      return 1;
    }
  } 

  while ((read = getline(&screen_name, &len, f)) != -1)
  {
    if (strlen(screen_name) > 16 )
    {
      ttwytter_output(stderr, "Error: Username must be less than 16 characters. Use -q to suppress this warning.\n", screen_name);
      fflush (f);
    }
    else 
    {
      if ((response_string = ttwytter_get_data(remove_first_char(screen_name))) != NULL) /* get the tweet with the username set with -u*/
      {
        if ((parsed_struct = ttwytter_parse_json(response_string)) != NULL)
        {
          ttwytter_output_data(parsed_struct);
        }
        else
        {
          ttwytter_output(stderr, "Error parsing data."); 
        }
      }
      else
      {
          ttwytter_output(stderr, "Error retreiving data.");
      }
    }
  }
  
  free(screen_name);
  fclose(f);

  return 0;
}

void ttwytter_output(FILE *stream, const char *format, ...) /* outputs text. It's written in this way to allow the q-flag in a simple way. */
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
    ttwytter_output(stderr, "Not enough memory (realloc returned NULL in write_to_memory() )\n");

    return 0;
  }
 
  memcpy(&(mem->memory[mem->size]), response, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}

char *remove_first_char(char* string)
{
    if (*string == '@' || *string == '#')
    {
        return string + 1;
    }
    else
    {
        return string;
    }
}

int ttwytter_stream() /* This functions listens to any new tweets coming from the given username, given by -u */
{
  Data *parsed_struct = NULL;
  Data *parsed_struct_first = NULL;
  char *temp = NULL; /* This is a hack, so I can copy the strings from the structs, as the are const */

  temp = malloc(sizeof(char) * TWEET_SIZE);

  char* response = NULL;
  char* first_response = NULL;

  /* Set up the select() function */
  fd_set input_set;
  struct timeval timeout;
  int ready_for_reading = 0;

  /* Waiting for some seconds */
  timeout.tv_sec = 30;    /* 30 seconds is the default */
  timeout.tv_usec = 0;    /* 0 milliseconds */

  char input;
  int len;

  ttwytter_output(stderr, "Listening to stream @%s. Use 'ctrl-D' to send EOF and break the stream.\n", screen_name);
  first_response = ttwytter_get_data(screen_name); /* get the last tweet to compare with what we get to the stream. We only print new tweets. */
  parsed_struct_first = ttwytter_parse_json(first_response);

  strlcpy(temp, parsed_struct_first[0].id_str, TWEET_SIZE);

   while (input != EOF)
   {
        /* Selection */
        FD_ZERO(&input_set );   /* Empty the FD Set */
        FD_SET(0, &input_set);  /* Listen to the input descriptor */
        int ready_for_reading = select(1, &input_set, NULL, NULL, &timeout);

        /* Selection handling */
        if (ready_for_reading < 0)
        {
            ttwytter_output(stderr, "select() returned an error.\n");

            return 1;
        }
        else if (ready_for_reading)
        {
            input = fgetc(stdin);  
        }

        // if (input != EOF)
        // {
          if ((response = ttwytter_get_data(screen_name)) != NULL) /* get the tweet with the username set with -u*/
          {
            if ((parsed_struct = ttwytter_parse_json(response)) != NULL)
            {
              if (strcmp(parsed_struct[0].id_str, temp) != 0)
              {
                ttwytter_output_data(parsed_struct);
                strlcpy(temp, parsed_struct[0].id_str, 512);
              }
          }
          // }
          ttwytter_output(stderr, ".\n");
        }     
   }

   free(parsed_struct_first);
   free(temp);

   return 0;
}

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


