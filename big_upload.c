// Copyright (c) 2015 Cesanta Software Limited
// All rights reserved
//
// This example demonstrates how to handle very large requests without keeping
// them in memory.

//gcc mongoose.c big_upload.c -DMG_ENABLE_HTTP_STREAMING_MULTIPART
//./webfile_server -d /file_share

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mongoose.h"

static struct mg_serve_http_opts s_http_server_opts;

static const char *s_http_port = "8000";

struct file_writer_data {
  FILE *fp;
  char *filename;
  size_t bytes_written;
};


static void handle_request(struct mg_connection *nc) {
  // This handler gets for all endpoints but /upload
  mg_printf(nc, "%s",
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<html><body>Upload example."
            "<form method=\"POST\" action=\"/upload\" "
            "  enctype=\"multipart/form-data\">"
            "<input type=\"file\" name=\"file\" /> <br/>"
            "<input type=\"submit\" value=\"Upload\" />"
            "</form></body></html>");
  nc->flags |= MG_F_SEND_AND_CLOSE;
}

static const struct mg_str s_get_method = {"GET",3};
static const struct mg_str s_put_method = {"POST",4};
static void send_err(struct mg_connection *nc,char *msg)
{
  mg_printf(nc, 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n"
            "\r\n"
	    "%s",msg);
//  nc->flags |= MG_F_SEND_AND_CLOSE;

}
static void handle_execute(struct mg_connection *nc, int ev, void *_p) {
  char buf[4096] = {0};
  FILE *output = NULL;
  int len;
  struct http_message  *http_msg = (struct http_message *)_p;
#define is_equal(a,b) (!strncmp((a)->p,(b)->p,(a)->len))
  int ret = mg_get_http_var(is_equal(&http_msg->method,&s_get_method)?&http_msg->query_string:&http_msg->body,
		  "cmd",buf,sizeof(buf)-1);
  if(ret>0){
  	printf("execute cmd:%s \n",buf);
	output = popen(buf,"r");
	if(!output){
		send_err(nc,"execute err\n");
	}else {
		while(fgets(buf,sizeof(buf),output)){
			//mg_send(nc,buf,len);
			mg_printf(nc,"%s",buf);
			//mg_flush
		}
	}
  }

  if(output)
	  pclose(output);

  nc->flags |= MG_F_SEND_AND_CLOSE;
}

static void handle_upload(struct mg_connection *nc, int ev, void *_p) {
  struct file_writer_data *data = (struct file_writer_data *) nc->user_data;
  struct mg_http_multipart_part *mp = (struct mg_http_multipart_part *) _p;
  char filename [512] = {0};
  char abs_path [600] = {0};
  char *p,*strip_name=filename;
  time_t _time;
  struct tm tm; 
  FILE *fp;

  //printf("%d %p %p %p\n",ev,nc,_p,data); 
  switch (ev) {
    case MG_EV_HTTP_PART_BEGIN: {
      if (data == NULL)  {
	//get uploader filename;
	if(mp->file_name){
		snprintf(filename,1024,"%s",mp->file_name);
		for(p=filename;*p;p++){
			if(*p=='\\' || *p=='/'){
				strip_name = p+1;
			}
		}
	}else{
		time(&_time);
		localtime_r(&_time,&tm);
		strftime(strip_name,1024-(strip_name-filename),"unknow_upload_file_%Y_%m_%d_%H_%M",&tm);
	}
	printf("Upload filename:%s \n",strip_name);
	 
	snprintf(abs_path,600,"%s/%s",s_http_server_opts.document_root,strip_name);
	if(!strcmp(strip_name,s_http_server_opts.directory_listing_insert_js) ){
          	nc->flags |= MG_F_SEND_AND_CLOSE;
		break;
	}

        fp = fopen(abs_path,"wb+");
	if(!fp){
		nc->flags |= MG_F_SEND_AND_CLOSE;	
		break;
	}
        data = calloc(1, sizeof(struct file_writer_data));
	data->fp = fp;
	data->filename = strdup(strip_name);
        data->bytes_written = 0;

        if (data->fp == NULL) {
          mg_printf(nc, "%s",
                    "HTTP/1.1 500 Failed to open a file\r\n"
                    "Content-Length: 0\r\n\r\n");
          nc->flags |= MG_F_SEND_AND_CLOSE;
          return;
        }
        nc->user_data = (void *) data;
      }
      break;
    }
    case MG_EV_HTTP_PART_DATA: {
      if(!data){
        nc->flags |= MG_F_SEND_AND_CLOSE;
      	break;//error.
      }
      if (fwrite(mp->data.p, 1, mp->data.len, data->fp) != mp->data.len) {
        mg_printf(nc, "%s",
                  "HTTP/1.1 500 Failed to write to a file\r\n"
                  "Content-Length: 0\r\n\r\n");
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
      }
      data->bytes_written += mp->data.len;
      break;
    }
    case MG_EV_HTTP_PART_END: {
      if(!data){
        nc->flags |= MG_F_SEND_AND_CLOSE;
      	break;//error.
      }
      mg_printf(nc,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n"
                "Written %ld of POST data to file %s \n\n",
                (long) ftell(data->fp),data->filename);
      //mg_send_http_chunk(nc,"",0);//not close???
      nc->flags |= MG_F_SEND_AND_CLOSE;
      fclose(data->fp);
      free(data->filename);
      free(data);
      nc->user_data = NULL;
      system("sync");
      break;
    }
  }
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
  (void) ev_data;
  switch (ev) {
    case MG_EV_HTTP_REQUEST:
      // Invoked when the full HTTP request is in the buffer (including body).
      //handle_request(nc);
      mg_serve_http(nc,ev_data,s_http_server_opts);
      break;
  }
}

int usage(char *prog,int ret)
{
	printf("Usage:%s (-b|--bind) bindaddr (-d|--directory) root_webdir \n"
	       "         -b bindaddr default: 0.0.0.0:80 \n"
	       "         -d root_webdir default: . \n",
	       prog);
	return;
}

int main(int argc,char *argv[]) {
  struct mg_mgr mgr;
  struct mg_connection *nc;
  char *bindaddr = "0.0.0.0:80";
  char *root_dir = ".";
  int i;
  if(argc >= 2){
	#define strcmp(a,b) strncmp(a,b,strlen(b))
  	for(i=1;i<argc;i++){
		if(!strcmp(argv[i],"-h") || !strcmp(argv[i],"--help")){
			return usage(argv[0],0);
		}else if(!strcmp(argv[i],"-b")||!strcmp(argv[i],"--bind")){
			if(++i >=  argc) 
				return usage(argv[0],1);
			bindaddr = argv[i];	
		}else if(!strcmp(argv[i],"-d") || !strcmp(argv[i],"--directory")){
			if(++i >=  argc) 
				return usage(argv[0],1);
			root_dir = argv[i];
		}
	}
  }


  mg_mgr_init(&mgr, NULL);
  nc = mg_bind(&mgr, bindaddr , ev_handler);

  mg_register_http_endpoint(nc, "/upload", handle_upload);
  mg_register_http_endpoint(nc, "/execute", handle_execute);
  // Set up HTTP server parameters
  mg_set_protocol_http_websocket(nc);

  s_http_server_opts.document_root = root_dir ;
  s_http_server_opts.enable_directory_listing = "yes" ;
  s_http_server_opts.directory_listing_insert_js = "/helper.js" ;

  printf("Starting web server on port %s, root_dir:%s \n", bindaddr,root_dir);
  for (;;) {
    mg_mgr_poll(&mgr, 1000);
  }
  mg_mgr_free(&mgr);

  return 0;
}
