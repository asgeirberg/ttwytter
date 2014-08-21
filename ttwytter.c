#include "ttwytter.h"

int main (int argc, char **argv)
{
  char *response_string;
  Data *parsed_struct = NULL;

  curl_global_init(CURL_GLOBAL_ALL);

  if (parse_arguments_long(argc, argv) != 0)
  {
    return 1;
  }

  if (ttwytter_set_user(argc, argv) != 0)  /* Set the authenticating user */
  {
    ttwytter_output(stderr, "Couldn't set user.\n");

    return 1;
  }

  if (print_user_flag)
  {
    ttwytter_output(stdout, "Authenticated as @%s.\n", user.screen_name);
  }
  
  if (user_flag || file_flag || mentions_flag || timeline_flag || search_flag) /* By activating any of the 'get-data' flags, we go into that mode. */
  {
    if (file_flag)
    {
      ttwytter_read_from_file(filename, f);
    }
    else if (user_flag || mentions_flag || timeline_flag || search_flag)
    {
      if (stream_flag) /* Stream supersedes everything else */
      {
        ttwytter_stream();
      }
      else
      {

        if ((response_string = ttwytter_get_data(query)) != NULL) /* get the tweet with the username set with -u */
        {
          if ((parsed_struct = ttwytter_parse_json(response_string)) != NULL) /* Parse the arguments using the output from get_data() */
          {
            ttwytter_output_data(parsed_struct);
          }
          else
          {
            ttwytter_output(stderr, "Error parsing data.\n");
          }
        }
        else
        {
          ttwytter_output(stderr, "Error retreiving data.\n");
        }
      }
    }
  }
  else if (read_from_stdin) /* if nothing is indicated by the user, we listen to stdin */
  {  
      ttwytter_output(stderr, "Reading from stdin. Use 'ctrl-D' to send EOF\n"); 
      ttwytter_read_from_file(NULL, stdin);
  }  

  if (post_tweet_flag || destroy_tweet_flag)
  {
    //printf("%s\n", postdata); /* This is here for debugging purposes */
    int curl_status = ttwytter_post_data(postdata);
  }

  curl_global_cleanup(); /* do the cleaning up for libcurl */

  free(count); /* this also frees the count set in the parse_arguments-function */
  free(response.memory);
  free(parsed_struct);
    
  return 0;
}