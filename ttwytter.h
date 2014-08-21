#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <oauth.h>
#include <curl/curl.h>
#include <stdarg.h>
#include <jansson.h>

#define FORMAT_SIZE 5000
#define TWEET_SIZE 512

#define USER_KEYFILE "user.keyfile"

/* Paths to the appropriate Twitter API calls.*/ 
#define USER_TIMELINE "https://api.twitter.com/1.1/statuses/user_timeline.json" /* gets last n tweets from a given user*/
#define MENTIONS_TIMELINE "https://api.twitter.com/1.1/statuses/mentions_timeline.json" /* gets the last n mentions of the authenticated user*/
#define HOME_TIMELINE "https://api.twitter.com/1.1/statuses/home_timeline.json" /* gets the last n tweets from the authenticated user's timeline*/
#define POST_TWEET "https://api.twitter.com/1.1/statuses/update.json" /* posts tweets */
#define DESTROY_TWEET "https://api.twitter.com/1.1/statuses/destroy/" /* destroy tweets */
#define SEARCH_TWEETS "https://api.twitter.com/1.1/search/tweets.json" /* search for tweets*/

/* Paths to the appropriate oauth urls.*/ 
#define REQUEST_TOKEN "https://api.twitter.com/oauth/request_token" /* Request an oauth token.*/
#define AUTHENTICATE_URL "https://api.twitter.com/oauth/authenticate" /* URL the user needs to visit */
#define ACCESS_URL "https://api.twitter.com/oauth/access_token" /* Sends the pin to reiceive the final tokens. */

/* API keys for the program*/
#define CONSUMER_KEY "ztmB7Bda6O3EJcz1XYEC3NILX"
#define CONSUMER_SECRET "PsF8UXmCit7vReiLmWI2oP8uzyK9yJd527aGwPuWsf46dG3SJJ"

/* This struct is for information about the authenticating user */
struct User {
  char screen_name[64];
  char user_token[64];
  char user_secret[64];
};

struct User user;

/* command line flags and arguments */
static int follow_flag, unfollow_flag; /* These are different because the have no short option associated with them */
unsigned short int alert_flag = 0; /* plays alerts */
unsigned short int user_flag = 0;
unsigned short int search_flag = 0;
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
unsigned short int print_user_flag = 0;
char *filename;
char *postdata;

/* this is used by write_to_memory() to reserve memory for the response we get from the Twitter API. */
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
char *query;
char *count;

FILE *f = NULL;

/* main functions */
int ttwytter_set_user(int argc,  char **argv); /* sets up the authenitcated user */
int ttwytter_authenticate(); /* authenticates the user */
char *ttwytter_request(char *http_method, char *url, char *url_enc_args);
char *ttwytter_get_data(char *query);
int ttwytter_post_data(char *tweet);
Data *ttwytter_parse_json(char *response); //parses the output from libcurl.
int ttwytter_read_from_file(char *filename, FILE *f); /*reads input from file when -f is used. stdin is passed as the file pointer when -f is not used  */
int parse_arguments(int argc, char **argv);
int parse_arguments_long(int argc, char **argv);

/* auxiliary functions */
char *ttwytter_build_url(char *query); 
char *ttwytter_build_header(char *url, char *url_enc_args);
int twytter_check_curl_status(int curlstatus);
void ttwytter_output(FILE *stream, const char *output, ...);
size_t static write_to_memory(void *response, size_t size, size_t nmemb, void *userp);
char *remove_first_char(char* string);
int twytter_get_char(void);

int parse_arguments_long(int argc, char **argv)
{
  int c; 

       while (1)
         {
           static struct option long_options[] =
             {

              {"follow",     no_argument,          &follow_flag,   1},
              {"unfollow",   no_argument,          &unfollow_flag, 1},
               /* These options don't set a flag.
                  We distinguish them by their indices. */
              {"alert",      no_argument,        0, 'a'},
              {"count",      required_argument,  0, 'c'},
              {"delete",     required_argument,  0, 'd'},
              {"search",     required_argument,  0, 'e'},
              {"file",       required_argument,  0, 'f'},
              {"auth",       no_argument,        0, 'g'},
              {"help",       no_argument,        0, 'h'},
              {"id",         no_argument,        0, 'i'},
              {"timeline",   no_argument,        0, 'l'},
              {"mentions",   no_argument,        0, 'm'},
              {"post",       optional_argument,  0, 'p'},
              {"nometadata", no_argument,        0, 'Q'},
              {"quiet",      no_argument,        0, 'q'},
              {"stream",     no_argument,        0, 's'},
              {"time",       no_argument,        0, 't'},
              {"user",       required_argument,  0, 'u'},
              {"verbose",    no_argument,        0, 'v'},
              {0, 0, 0, 0}
             };
           /* getopt_long stores the option index here. */
           int option_index = 0;
     
           c = getopt_long_only (argc, argv, "ac:d:e:f:ghilmp:Qqstu:v",
                            long_options, &option_index);
     
           /* Detect the end of the options. */
           if (c == -1)
             break;
     
           switch (c)
             {
             case 0:
               /* If this option set a flag, do nothing else now. */
               if (long_options[option_index].flag != 0)
                 break;
               printf ("option %s", long_options[option_index].name);
               if (optarg)
                 printf (" with arg %s", optarg);
               printf ("\n");
               break;
     
             case 'a': 
              alert_flag = 1; 

              break;
            case 'c': /* at present -c also accept strings as arguments. This should be restricted to integers. (The type of the argument is always a string) */
              if (strlen(optarg) < 5)
              {
                count = malloc(sizeof(char) * 4);
                strcpy(count, optarg);
              }
              else
              {
                printf("-c only accepts arguments up to 9999.\n");

                return 1;
              }

              break;
            case 'd':
              destroy_tweet_flag = 1;
              postdata = optarg;

              break;
            case 'e':
              search_flag = 1;
              printf("%s\n", query);
              query = optarg;

              break; 
            case 'f':
              file_flag = 1;
              filename = optarg;

              break;
            case 'g':
              print_user_flag = 1;

              break;
            case 'h':
              printf("usage: ttwytter -c <number> -u <user name> -f <file> -p <\"tweet\"> -ahilmtQqsv\n");
              exit(EXIT_SUCCESS);

              break;
            case 'i':
              id_flag = 1;

              break; 
            case 'l':
              timeline_flag = 1;

              break;
            case 'm':
              mentions_flag = 1;

              break;
            case 'p':
              post_tweet_flag = 1;

              if (strlen(optarg) < 141)
              {
                postdata = optarg;
              }
              else
              {
                ttwytter_output(stderr, "%s is more than 140 characters long.\n", optarg); /* Test this properly, Twitter counts this differently.*/
                exit(EXIT_SUCCESS);
              }

              break;
            case 'Q': /* Causes only text to be output, no metadata */
              supress_output_flag = 1;

              break;
            case 'q': 
              quiet_flag = 1; 

              break;
            case 's':
              stream_flag = 1;
        
              break;
            case 't':
              time_flag = 1;

              break;
            case 'u':
              user_flag = 1;

              if (strlen(optarg) < 16) /* check the length of the input. Twitter only allows 15 chars. */
              {
                query = remove_first_char(optarg);
              }
              else
              {   
                ttwytter_output(stderr, "%s is too long. Only 15 characters are allowed.\n", optarg);
              
                exit(EXIT_SUCCESS);
              }

              break;
            case 'v':
              verbose_flag = 1;
     
            case '?':
              printf("usage: ttwytter -c <number> -u <user name> -f <file> -p <\"tweet\"> -ahilmtQqsv\n");
              exit(EXIT_FAILURE);

            /* getopt_long already printed an error message. */
              break;
     
            default:
              exit(EXIT_FAILURE);
            }
         }
     
       /* Print any remaining command line arguments (not options). */
       if (optind < argc)
         {
           printf ("non-option ARGV-elements: ");
           while (optind < argc)
             printf ("%s ", argv[optind++]);
           putchar ('\n');

           return 1;
         }
     
       return 0;
}

int ttwytter_set_user(int argc, char **argv)
{

  FILE *f, *g;
  struct User temp;

  if ( access(USER_KEYFILE, F_OK ) == -1 )
  {
    ttwytter_output(stderr,"Keyfile not found. Authenticate with Twitter? y/n\n");
    fflush (stdout);

    int c = twytter_get_char();

    if (c == 'y')
    {
      ttwytter_authenticate();

      if ((f = fopen(USER_KEYFILE, "w")) != NULL)
      {
        fwrite(&user, sizeof(struct User), 1, f);
      }

      fclose(f);
    }
    else
    {
      return 1;
    }
  }
  else
  {
    if ((f = fopen(USER_KEYFILE, "r")) != NULL)
    {
      fread(&user, sizeof(struct User), 1, f);
    }
    fclose(f);
  }
  
  return 0;
}

int ttwytter_authenticate() /* This function will authenticate the user */ 
{
  char *request_response, *authenticate_response, *request_token, oauth_verifier[64];
  char **argv = malloc(0);
  int argc, pincode;

  request_response = ttwytter_request("POST", REQUEST_TOKEN, "oauth_callback=oob"); /* This doesn't actally mean that the method is "POST". */
  argc = oauth_split_url_parameters(request_response, &argv);                       /* Just that it needed to share code with the rest of the POST code. */
                                                                                    /* request() should be rewritten to a more natural structure.*/ 
                                                                                         
  /* temporary set the user access token for the final auth-call. These will be replaced by the final tokens after the pin is sent. */
  strcpy(user.user_token, (strrchr(argv[0], '=') + 1));
  strcpy(user.user_secret, (strrchr(argv[1], '=') + 1));
       
  ttwytter_output(stdout, "To authorize, visit this URL, log in to your Twitter account, and enter the pin provided:\n" );
  ttwytter_output(stdout, "%s?%s\n", AUTHENTICATE_URL, argv[0]);
  ttwytter_output(stdout, "Enter PIN: " );
  scanf("%d", &pincode ); /* Needs to be made safer, without scanf and verifying input. */ 

  sprintf(oauth_verifier, "oauth_verifier=%d", pincode );

  for (int i = 0; i < argc; i++)
  {
    free(argv[i]);
  }
  free(argv);

  authenticate_response = ttwytter_request("POST", ACCESS_URL, oauth_verifier);

  /* Copies the correct user keys from the response. This is very sensitive to the output of the response. A better parsing function might be better.*/

  argv = malloc(0);
  argc = oauth_split_url_parameters(authenticate_response, &argv);  

  strcpy(user.user_token, (strrchr(argv[2], '=') + 1));
  strcpy(user.user_secret, (strrchr(argv[3], '=') + 1)); 
  strcpy(user.screen_name, (strrchr(argv[5], '=') + 1));

  ttwytter_output(stderr, "Authentication successful. Authenticated as @%s.\n", user.screen_name); /* Needs to do more robust error checking. */

  for (int i = 0; i < argc; i++)
  {
    free(argv[i]);
  }
  free(argv);                                               

  return 0;
}

char *ttwytter_get_data(char *query)
{
  if (user_flag || file_flag)
  {
    return ttwytter_request("GET", USER_TIMELINE, query);
  }
  else if (mentions_flag) /* Needs to check if any user is authenticated */
  {
    return ttwytter_request("GET", MENTIONS_TIMELINE, query);
  }
  else if (timeline_flag) /* Needs to check if any user is authenticated */
  {
    return ttwytter_request("GET", HOME_TIMELINE, NULL);
  }
  else if (search_flag)
  {  
    return ttwytter_request("GET", SEARCH_TWEETS, query);
  }

  return NULL;
}

int ttwytter_post_data(char *postdata)
{
  char *url_enc_args = NULL;

  if (post_tweet_flag) /* Needs to check if any user is authenticated */
  {
    url_enc_args = malloc(sizeof(char) * 512);

    sprintf(url_enc_args, "status=%s", postdata);
    ttwytter_request("POST", POST_TWEET, oauth_url_escape(url_enc_args));

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

char *ttwytter_request(char *http_method, char *url, char *url_enc_args) /* TODO: rewrite this to a more modular design, too much code is being reused. */
{
  CURL *curl = curl_easy_init();

  if (!strcmp(http_method, "GET"))
  {
    response.memory = malloc(1);  /* will be grown as needed by using realloc */ 
    response.size = 0;    /* no data at this point */

    char *url = ttwytter_build_url(url_enc_args); 
    char *signedurl = oauth_sign_url2(url, NULL, OA_HMAC, http_method, CONSUMER_KEY, CONSUMER_SECRET, user.user_token, user.user_secret);
    curl_easy_setopt(curl, CURLOPT_URL, signedurl); /* URL we're connecting to, after being signed by oauthlib */
    
    free(url);

    if (verbose_flag)
    {
      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    }
  }
  else if (!strcmp(http_method, "POST")) /* The code to build the header should be in its own function.*/
  {
    if (post_tweet_flag || !strcmp(url, REQUEST_TOKEN) || !strcmp(url, ACCESS_URL))
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
      argc = oauth_split_post_paramters(ser_url, &argv, 1);

      free(ser_url);

      if (!strcmp(url, REQUEST_TOKEN))
      {
        http_method = "GET";
        temp_url = oauth_sign_array2(&argc, &argv, NULL, OA_HMAC, http_method, CONSUMER_KEY, CONSUMER_SECRET, NULL, NULL);
        free(temp_url);

      }
      else if (!strcmp(url, ACCESS_URL))
      {
        http_method = "GET";
        temp_url = oauth_sign_array2(&argc, &argv, NULL, OA_HMAC, http_method, CONSUMER_KEY, CONSUMER_SECRET, user.user_token, user.user_secret);
        free(temp_url);
      }
      else
      {
        temp_url = oauth_sign_array2(&argc, &argv, NULL, OA_HMAC, http_method, CONSUMER_KEY, CONSUMER_SECRET, user.user_token, user.user_secret);
        free(temp_url);
      }
      
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
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);


      if (!strcmp(http_method, "POST"))
      {
        curl_easy_setopt(curl, CURLOPT_POST, 1 );
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata);
      }
      
      if (verbose_flag)
      {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
      }
    }  
  }
  else
  {
    ttwytter_output(stderr, "Invalid http-method.\n");

    return NULL;
  }
  
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "ttwytter/0.4"); /* User agent we're going to use */
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1); /* libcurl will now fail on an HTTP error (>=400) */
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_memory); /* setting a callback function to return the data */
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response); /* passing the pointer to the response as the callback parameter */ 

  int curlstatus = curl_easy_perform(curl); /* Execute the request! */

  curl_easy_cleanup(curl);

  if (twytter_check_curl_status(curlstatus))  /* Handles error messages from cURL */
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
  char *screen_name = malloc(sizeof(char) * 16);

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

  ttwytter_output(stderr, "Listening to stream @%s. Use 'ctrl-D' to send EOF and break the stream.\n", query);
  first_response = ttwytter_get_data(query); /* get the last tweet to compare with what we get to the stream. We only print new tweets. */
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
          if ((response = ttwytter_get_data(query)) != NULL) /* get the tweet with the username set with -u*/
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

char *ttwytter_build_url(char *screen_name) //screen name is outdated as variable name and only causes confusion, change to query
{
  char *url = malloc(sizeof(char) * 256); /* get rid of this magic numner */

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
  else if (search_flag)
  {
    strcpy(url, SEARCH_TWEETS);
    strcat(url, "?q=");
    strcat(url, screen_name);
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

int twytter_check_curl_status(int curlstatus)
{
  if (curlstatus != 0) /* this might be rewritten with a case structure, to better distinguish errors */
    {
      if (curlstatus == 22)
      {
        ttwytter_output(stderr, "No such user as %s.\n", query); 
      }
      else 
      {
        ttwytter_output(stderr, "curl_easy_perform terminated with status code %d\n", curlstatus); 
      }  
      return 1;
    }
  return 0;  
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

int twytter_get_char(void)
{
    int c;
    int answer = 0;
    while ((c = getchar()) != EOF && c != '\n')
    {
        if (answer == 0 && (c == 'y' || c == 'n' || c == 'N' || c == 'Y'))
        {
          answer = c;
        } 
        else
        {
          /* ?check for garbage here and complain? */
        }
    }
    return answer;
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


