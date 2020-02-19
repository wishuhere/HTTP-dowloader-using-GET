#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdarg.h>

//Thu vien tao thu muc
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFFERSIZE 4096
#define PORT 80
#define USERAGENT "HTMLGET 1.0"

struct stat st = {0};

char *getlink(FILE *fp)
{
       //Source: Stack over flow
       //Dung de lay link tu the href
	if(fp == NULL)
	{
		printf("File poiter is NULL\n");
		return NULL;
	}

	char *link = NULL;
	//char *pt1, *pt2;
	char *pt1 = NULL, *pt2 = NULL;
	char buffer[BUFFERSIZE];
	
	while(fgets(buffer,BUFFERSIZE,fp))
	{

		if((pt1 = strstr(buffer,"href=\""))!=NULL)
		{
			pt1+=6;		//Skip chu href
			pt2 = strtok(pt1,"\"");
			break;
		}
	}
	
	if(pt2!=NULL)
	{
		link = (char*) malloc(strlen(pt2)+1);
		strcpy(link,pt2);
		return link;
	}
	return NULL;
}

char *get_host(char *path)
{
	//Input: http://abc.com.vn/.....
	//Output: abc.com
	char *str = (char*) malloc(strlen(path)+1);
	strcpy(str,path);
	char *temp;
	char *host;
	
	temp = strtok(str,"/");
	if (temp && !strcmp(temp,"http:"))
	{
		temp = strtok(NULL,"/");
	}
	host = (char*) malloc(strlen(temp)+1);
	strcpy(host,temp);
	free(str);
	return host;
}

char *get_page(char *path, char *host)
{
        //Input: http://..../abc.../
	//Output: abc.../
	printf("\n");
	char *temp;
	char *page = NULL;
	char *str = (char*)malloc(strlen(path)+1);
	char *str2 = (char*)malloc(strlen(path)+1);
	char *temp2;
	
	strcpy(str,path);
	strcpy(str2,path);
	temp2 = str2;
	temp = strtok(str,"/");
	
	if ( strcmp(temp,"http:") == 0 )
	{
		str2 += 7 + strlen(host);  //+7 = skip "http://"
		str2[strlen(str2)] = '\0';
	}
	else
	{
		str2 += strlen(host);
	}
	
	page = (char*) malloc(strlen(str2)+1);
	strcpy(page,str2);
	free(str);
	free(temp2);
	return page;
}

char *get_filename(char *path)
{
	int check = 0;
	if(path[strlen(path)-1] == '/')	
		check = 1;	//Day la folder
		
	char *temp, *sav;
	char *filename;
	char *str = (char*) malloc(strlen(path)+1);
	strcpy(str,path);
	temp = strtok(str,"/");
	sav = temp;
	
	while(temp = strtok(NULL,"/")) 
        { 
              sav = temp; 
        }
	
	if(check == 1)
	{
		filename = (char*) malloc(strlen(sav)+2);
		strcpy(filename,sav);
		strcat(filename,"/");
	}
	else
	{
		filename = (char*) malloc(strlen(sav)+1);
		strcpy(filename,sav);
	}
	free(str);
	return filename;
}

char *get_request(char *host, char *page)
{
	char *request;
	char *getpage = page;
	char *tpl = "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n";
	
	//Bo dau '/'
	if(getpage[0] == '/')
	{
		getpage = getpage + 1;
	}
	request = (char *)malloc(strlen(host)+strlen(getpage)+strlen(USERAGENT)+strlen(tpl)-5);
	sprintf(request, tpl, getpage, host, USERAGENT);
	return request;
}


//Co 2 mode
//1: download 1 file le
//2: download file trong 1 folder
char *download_file(char *path, char *folder_name, int mode)
{
	//Get host, page
	char *host, *page, *filename;
	char *html_query;
	char ip[INET_ADDRSTRLEN];
	int DSock;
	int response;	//return code
	struct addrinfo hints;
	struct addrinfo *info;	//pointer to results

	host = get_host(path);
	page = get_page(path, host);
	filename = get_filename(path);
	printf("Get file name: %s\n",filename);

	//Make the struct empty!
	memset(&hints, 0, sizeof(hints));	
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	//Lay thong tin tu host
	response = getaddrinfo(host,"http",&hints,&info);
	if(response != 0)
	{
		
		fprintf(stderr,"Failed at getaddrinfo(): %s\n", gai_strerror(response));
		printf("Host: %s\n",host);
		printf("Page: %s\n",page);
		return NULL;
	}

	//Convert ip stored in info->ai_addr->sin_addr to char* ip with length INET_ADDRSTRLEN
	inet_ntop(info->ai_family, &(((struct sockaddr_in *)info->ai_addr)->sin_addr),ip,INET_ADDRSTRLEN);
	DSock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	 
	if(DSock == -1)
	{
		//Failed!
		printf("Cannot create socket!\n");
		//clear up struct addrinfo info!
		freeaddrinfo(info);	
		free(html_query);
		free(host);
		free(page);	
		return NULL;
	}
	 
	if(connect(DSock,info->ai_addr,info->ai_addrlen) < 0)
	{
		printf("Cannot connect to %s - %s\n",host,ip);
		//clear up struct addrinfo info!
		freeaddrinfo(info);	
		free(html_query);
		free(host);
		free(page);
		return NULL;
	}

	html_query = get_request(host, page);
	printf("Query content:\
						\n<<BEGIN>>\n%s\n<<END!>>\n",html_query);
	
	//Send html request
	//Loop toi khi goi dc het goi tin qua ben server
	int bytes_sent;
	response =  0;
	do
	{
		bytes_sent = send(DSock,html_query,strlen(html_query),0);
		if(bytes_sent == -1)
		{
			printf("Failed to send HTML-request\n");
			//clear up struct addrinfo info!
			freeaddrinfo(info);	
			free(html_query);
			free(host);
			free(page);	
			return NULL;
		}
		response += bytes_sent;
		
	}while(response < strlen(html_query));
	printf("Send HTML-REQUEST succeed\n");

	//clear up struct addrinfo info!
	freeaddrinfo(info);	
	free(html_query);
	free(host);
	free(page);	

	//###Receive response from server###
	FILE *fp;
	char *temp_name; 
	
	if(folder_name != NULL)
	{
		printf("Folder name: %s\n",folder_name);
		temp_name = (char*)malloc(strlen(filename) + strlen("1612070") + strlen(folder_name) + 10);
		strcpy(temp_name,folder_name);
	}
	else
	{
		//Chi down file
		temp_name = (char*)malloc(strlen(filename) + strlen("1612070") + 2);
		strcpy(temp_name,"1612070");
		strcat(temp_name,"_");
		if(filename[strlen(filename)-1] == '/')
		{
			printf("This is a folder!\n");
			filename[strlen(filename)-1] = '\0';
		}
	}
	strcat(temp_name,filename);

	if(temp_name[strlen(temp_name)-1] == '/')
		temp_name[strlen(temp_name)-1] = '\0';
	
	
	fp = fopen(temp_name,"w");
	
	if (fp == NULL)
	{
		printf("Failed to write file: %s\n",temp_name);
		return NULL;
	}

	char buffer[BUFFERSIZE];
	memset(buffer, 0, sizeof(buffer));
	int bytes_recv;
	long total = 0;
	long total_write = 0;
	int htmlstart = 0;
	char *htmlcontent;

	while((bytes_recv = recv(DSock,buffer,BUFFERSIZE,0)) > 0)
	{
		total+=bytes_recv;	
		if(htmlstart == 0)
		{
			//Skip header!!!
			htmlcontent = strstr(buffer,"\r\n\r\n");
			int header_size = strlen(buffer) - strlen(htmlcontent) + 4;
			if(htmlcontent != NULL)
			{
				htmlstart = 1;
				htmlcontent += 4;		//Skip \r\n\r\n
				int real_bytes = bytes_recv - header_size;
				total_write += fwrite (htmlcontent , sizeof(char), real_bytes, fp);
				memset(buffer,0,bytes_recv);
				continue;
			}
		}
		else
		{
			htmlcontent = buffer;
		}

		if(htmlstart == 1)
		{
			//Ghi file!
			total_write += fwrite (buffer, sizeof(char), bytes_recv, fp);
			printf("%s\n",htmlcontent);
		}
		memset(buffer,0,bytes_recv);
	}
	
	if(bytes_recv < 0)
	{
		printf("Error when receiving data...\n");
	}
	fclose(fp);
	printf("TOTAL: %ld bytes\n",total);
	printf("WROTE: %ld bytes\n",total_write);				
	//close socket
	close(DSock);	
	return temp_name;
}

int handle_download(char *path)
{
	//Get host, page
	char *host, *page, *filename;
	char *query;
	char *buffer;
	host = get_host(path);
	page = get_page(path, host);
	filename = get_filename(path);
	
	//Tao folder va luu file vao folder
	char *folder_name = NULL;
	if(filename[strlen(filename) - 1] == '/')
	{
		char *tmp = strdup(filename);
		tmp[strlen(tmp) - 1] = '\0';
		folder_name = (char*)malloc(strlen(tmp) + 4);		
		//+4 vi "./" voi '\0' va '/';
		strcpy(folder_name,"./");
                strcat(folder_name,"1612070");
                strcat(folder_name,"_");
		strcat(folder_name,tmp);
		strcat(folder_name,"/");		 
		if (stat(folder_name, &st) == -1) 
		{
			if(!mkdir(folder_name, 0777))
				printf("Created new directory\n");
			else
			{
				printf("Cannot create new directory\n");
				exit(EXIT_FAILURE);
			}
		}
		free(tmp);
	}	

        //Download 1 file hoac download 1 folder
	char *main_file = download_file(path, folder_name, 1);
	
	//Kiem tra down file hay folder
	int flag = 1; //1 la down 1 file, 0 la down folder
	if(filename[strlen(filename)-1] == '/')
	{
		flag = 0;
	}
	if (flag == 0)
	{		
		printf("Download folder!\n");
		//Lay link download tu the href
		FILE *fp = fopen(main_file,"r");
		if(fp == NULL)
		{
			printf("Error: Cannot open file!!\n");
			return -1;
		}

		char *short_link;
		
		while( !feof(fp))
		{
			if( (short_link = getlink(fp)) == NULL )
			{
				break;
			}
			//Download file con
			char *full_link = (char*)malloc(strlen(host) + strlen(page) + strlen(short_link) + 1);
			strcpy(full_link,host);
			strcat(full_link,page);
			strcat(full_link,short_link);
			
			if(full_link!=NULL)
				download_file(full_link, folder_name, 2);
			else
				printf("Link Error!\n");
	
			free(short_link);
			free(full_link);	
		};
		fclose(fp);
	}
	printf("\nDownload Completed\n");
	free(main_file);
	free(host);
	free(page);
	free(filename);
	free(folder_name);
	return 0;
}

int main(int argc, char **argv)
{
	if(argc == 1)
	{
		printf("Please insert the url");
                exit(EXIT_FAILURE);
	}

	handle_download(argv[1]);
	exit(EXIT_SUCCESS);
}
