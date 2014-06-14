#include "ttwytter.h"

int main (int argc, char **argv)
{
  response.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
  response.size = 0;    /* no data at this point */

  init_libcurl(argc, argv);  /* initalize the keys */
  curl_global_init(CURL_GLOBAL_ALL);
  CURL *curl = curl_easy_init();
  
  if (parse_arguments(argc, argv) != 0)
  {
    return 1;
  }

  if (get_tweet_flag == 1)
  {
    get_data(curl); /* get the tweet */
  }  

  curl_global_cleanup(); /* do the cleaning up for libcurl */
    
  return 0;
}

int parse_arguments(int argc, char **argv)
{
  int c;

  while ((c = getopt(argc, argv, "tc:qf:u:")) != -1)
  {
    switch (c)
    {
      case 'q': 
        quiet_flag = 1; 

        break;
      case 't':
        time_flag = 1;

        break;
      case 'u':
        get_tweet_flag = 1;

        if (strlen(optarg) < 16) //check the length of the input. Twitter only allows 15 chars.
        {
          screen_name = optarg;
        }
        else
        {
          output(stderr, "%s is too long. Only 15 characters are allowed.\n", optarg);
          return 1;
        }

        break;
      case 'f':
        filename = optarg;

        break;
      case 'c': //at present -c also accept strings as arguments. This should be restricted to integers. (The type of the argument is always a string)
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
      case '?':
        if (optopt == c)
        {
          output(stderr, "Option -%c requires an argument.\n", optopt);
        }
      return 1;
    }
  }
  return 0;
}


