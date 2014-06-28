#include "ttwytter.h"

int main (int argc, char **argv)
{
  ttwytter_init_libcurl(argc, argv);  /* initalize the keys */
  curl_global_init(CURL_GLOBAL_ALL);

  char *response_string;
  Data *parsed_struct = NULL;
  
  if (parse_arguments(argc, argv) != 0)
  {
    return 1;
  }

  if (user_flag || file_flag || mentions_flag || timeline_flag) /* By activating any of the 'get-data' flags, we go into that mode. */
  {
    if (file_flag == 1)
    {
      ttwytter_read_from_file(filename, f);
    }
    else if (user_flag || mentions_flag || timeline_flag)
    {
      if (stream_flag) /* Stream supersedes everything else */
      {
        ttwytter_stream();
      }
      else
      {

        if ((response_string = ttwytter_get_data(screen_name)) != NULL) /* get the tweet with the username set with -u*/
        {
          if ((parsed_struct = ttwytter_parse_json(response_string)) != NULL) /* Parse the arguments using the output from get_data() */
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
  }
  else if (user_flag == 0 && file_flag == 0 && mentions_flag == 0 && timeline_flag == 0)
  {  
      ttwytter_output(stderr, "Reading from stdin. Use 'ctrl-D' to send EOF\n"); 
      ttwytter_read_from_file(NULL, stdin);
  }  

  curl_global_cleanup(); /* do the cleaning up for libcurl */

  free(count); /* this also frees the count set in the parse_arguments-function */
  free(response.memory);
  free(parsed_struct);
    
  return 0;
}

int parse_arguments(int argc, char **argv) /* TODO: rewrite this with getopt-long */
{
  int c;

  while ((c = getopt(argc, argv, "ac:hlmtQqsu:f:")) != -1)
  {
    switch (c)
    {
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
      case 'f':
        file_flag = 1;
        filename = optarg;

        break;
      case 'h':
        printf("usage: ttwytter -c <number> -u <user name> -f <file> -ahlmtQqs\n");
        exit(EXIT_SUCCESS);

        break;
      case 'l':
        timeline_flag = 1;

        break;
      case 'm':
        mentions_flag = 1;

        break;
      case 'Q':
        supress_output_flag = 1;

        break;
      case 'q': 
        quiet_flag = 1; 

        break;
      case 't':
        time_flag = 1;

        break;
      case 'u':
        user_flag = 1;

        if (strlen(optarg) < 16) /* check the length of the input. Twitter only allows 15 chars. */
        {
          screen_name = remove_first_char(optarg);
        }
        else
        {
          ttwytter_output(stderr, "%s is too long. Only 15 characters are allowed.\n", optarg);
          return 1;
        }

        break;
      case 's':
        stream_flag = 1;
        
      break;
      case '?':
        printf("errror\n");

        if (optopt == c)
        {
          ttwytter_output(stderr, "Option -%c requires an argument.\n", optopt);
        }
      return 1;
    }
  }
  return 0;
}


