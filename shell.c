/* shell -ohjelma */
/* Tekijät: Juho Kontiainen 0503209 */
/*          Jesse Peltola   0523140 */
/* Lähteet: stackoverflow,https://brennan.io/2015/01/16/write-a-shell-in-c/, 
   kurssin esimerkit*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#define STRTOKSEP " \n"
#define MAXKOKO 1024
#define error_message "An error has occurred\n"

int suoritus(char **,char *,char *);
int prosessi(char **,char *,char *);
char* path(char**,char*);
int cd(char**);
char** builtins(void);
char* presuoritus(char**,char*,char*,char*);
char* parsefile(char*);
char** parse(char*);
char* luku();
char* batchluku(char*,char**);
void loop(char**);

int main(int argc, char **argv) {
  if (argc == 2) { //jos 2 argumenttia, luetaan batchfile
    loop(argv);
  } else if (argc == 1) { //jos yksi argumentti, tahdotaan vain ajaa shell
    loop(NULL);
  } else {
    fprintf(stderr,"Too many arguments given.\n"); //muuten liikaa argumentteja
  }
  return 0;
}
void loop(char **argv) {
  char *pathstr = malloc(MAXKOKO*sizeof(char));
  strcpy(pathstr,"/bin");
  int pituus = strlen(pathstr);
  pathstr[pituus+1] = 0;
  char **argw; /*argumentteja varten */
  char *rivi; 
  char *filen = NULL;
  if (argv != NULL) { /*batch */
    pathstr = batchluku(pathstr,argv); /*lukee ja käsittelee tiedoston sekä kutsuu
					 tarvittavia funktioita syötteen ajamiseen */
  }
  while(1) {	/*Idean lähde https://brennan.io/2015/01/16/write-a-shell-in-c/ */
    printf("> ");
    rivi = luku();
    filen = parsefile(rivi); /*Tiedoston nimi, jos on syötteessä on ">" */
    argw = parse(rivi);      /*whitespacen mukaan syötteen erottelu */  
    pathstr = presuoritus(argw,pathstr,rivi,filen);
/* Huom. exit on funktio erikseen, joka kutsuu exit(0), joten
   muistin vapautukset täytyy hoitaa ennen exitin kutsumista,
   sen takia annetaan pointtereina rivi, jota ei käytetä presuoritusfunktiossa */
  }
}
char* path(char **argw,char *pathstr) { /*muuttaa path-merkkijonoa sen mukaan mitä käyttäjä kutsuu -pathiä */
  int i;
  pathstr = malloc(MAXKOKO*sizeof(char));
  strcpy(pathstr,"");
  char t[] = ":";
  if (argw[1] == NULL) { /*jos annettu tyhjä */
    return pathstr;
  }
  for (i=1; argw[i] != NULL; ++i) {
    strcat(pathstr,argw[i]);
    strcat(pathstr,t);
  }
  return pathstr;
}
int cd(char **argw) { /*sisäänrakennettu cd */
  char *dir;
  if (argw[1] == NULL) {
    fprintf(stderr,"Too few arguments for command 'cd'.\n");
    return 1;
  }
  if (argw[2] != NULL) {
    fprintf(stderr,"Too many arguments for command 'cd'.\n");
    return 1;
  }
  dir = argw[1];
  if (chdir(dir) == -1) {
    fprintf(stderr,"cd error:\n");
    perror("cd");
  }
  return 0;
}
void lexit() { /*exit-funktio */
  exit(0);
}
char **builtints(void) { /*Sisäänrakennetut komennot,idean lähde osittain
													 https://brennan.io/2015/01/16/write-a-shell-in-c/  */
  char **builtins = malloc(MAXKOKO*sizeof(char));
  builtins[0] = "cd";
  builtins[1] = "path";
  builtins[2] = "exit";
  return builtins;
}
char* presuoritus(char **argw,char *pathstr,char *rivi,char *filen) { /*testaa halutaanko ajaa builtin-funktioita */
  int i;
  char **builtins = builtints();
  if (argw[0] == NULL) { /*syöte on tyhjä, joten ei suoriteta mitään */
    if (builtins) {free(builtins);} /*Muistin vapautusta */
    if (argw) {free(argw);}
    if (rivi) {free(rivi);}
    return pathstr;
  }
  for (i= 0; i<3; i++) { /*tarkistetaan löytyykö syöte builtin-funktioista */
    if (strcmp(argw[0], builtins[i]) == 0) { /*Löytyi */
      if (strcmp(builtins[i],"path") == 0) {
	if (builtins) {free(builtins);}
	if (pathstr) {free(pathstr);} /*vapautetaan muisti pathiä varten */
	pathstr = path(argw,pathstr);
	if (rivi) {free(rivi);}
	if (argw) {free(argw);}
	return pathstr;	
      } else if	(strcmp(builtins[i],"cd") == 0) {
	cd(argw);	
	if (builtins) {free(builtins);} /*Muistin vapautusta */
	if (argw) {free(argw);}
	if (rivi) {free(rivi);}
	return pathstr;	
      } else if (strcmp(builtins[i],"exit") == 0) {
	if (pathstr) {free(pathstr);} /*Muistin vapautusta */
	if (builtins) {free(builtins);}
	if (argw) {free(argw);}
	if (rivi) {free(rivi);}
	lexit();
      }
    }
  }
  /*Muuten liikutaan suoritus-funktioon */ 
  suoritus(argw,pathstr,filen);
  if (builtins) {free(builtins);} /*Muistin vapautusta */
  if (argw) {free(argw);}
  if (rivi) {free(rivi);}
  return pathstr;
}
int suoritus(char **argw,char *pathstr,char *filen) { /*Suorittaa(kutsuu prosessi-funktiota)
							jos access-kutsu löytää ohjelman  */
  int k,i = 0;
  char t[] = "/";
  char **slista = malloc(MAXKOKO*sizeof(char));
  if (slista  == NULL) {write(STDERR_FILENO, error_message, strlen(error_message));perror("malloc");}
  char *suorituspath = malloc(MAXKOKO*sizeof(char));
  if (suorituspath  == NULL) {write(STDERR_FILENO, error_message, strlen(error_message));perror("malloc");}
  char *token;
  char *pathstr2 = malloc(MAXKOKO*sizeof(char));
  if (pathstr2  == NULL) {write(STDERR_FILENO, error_message, strlen(error_message));perror("malloc");}
  strcpy(pathstr2,pathstr);
  token = strtok(pathstr2,":");  /*pathien parsiminen erilleen */
  while (token != NULL) { /*laitetaan pathit listaan slista */
    slista[i] = token;
    i++;
    token = strtok(NULL, ":");
  }  
  for (k=0; k<i; ++k) { /*path-listan käyminen läpi ja merkkijonon muokkaaminen
			  "oikeanlaiseksi" access- sekä execv-kutsuja varten */
    strcpy(suorituspath,slista[k]);
    strcat(suorituspath,t);    
    strcat(suorituspath,argw[0]); 
    if (!access(suorituspath,X_OK)) { /*jos löydetään ajettava ohjelma, lähde:
									http://pubs.opengroup.org/onlinepubs/009695299/functions/access.html*/      
      prosessi(argw,suorituspath,filen); /*ohjelma löytyi,ajetaan */
      if (slista) {free(slista);} /*Muistin vapautusta */
      if (suorituspath) {free(suorituspath);}
      if (pathstr2) {free(pathstr2);}
      if (token) {free(token);}
      return 0;
    }   
  }
  /*Muuten tänne */
  if (filen!= NULL) { /*virheen kirjoitus tiedostoon;ollaan epäonnistuttu ohjelman
			ajamisessa ja jos filen!=null, niin on yritetty kirjoittaa tiedostoon */
    FILE *filep;
    if ((filep=fopen(filen,"w")) == NULL) { /*Luo uuden tiedoston/tyhjentää vanhan,
					      en saanut tehtyä tätä open() kutsulla  */
      perror("fopen");
      exit(1);
    } else {
      fprintf(filep,error_message);
      fclose(filep);
      if (slista) {free(slista);} /*Muistin vapautusta */
      if (suorituspath) {free(suorituspath);}
      if (pathstr2) {free(pathstr2);}
      if (token) {free(token);}
      return 1;
    }
  }
  if (slista) {free(slista);} /*Muistin vapautusta */
  if (suorituspath) {free(suorituspath);}
  if (pathstr2) {free(pathstr2);}
  if (token) {free(token);}
  write(STDERR_FILENO, error_message, strlen(error_message));
  perror("access");
  return 1;
}
int prosessi (char **argw, char *pathstr,char *filen) { /*Kurssin tehtävien esimerkkirungosta fork-toiminta */
  int filefd;                                       	/*prosessi tekee lapsi-prosessin ja ajaa halutun ohjelman execv-kutsulla */
  FILE *filep;
  if (filen != NULL) {
    if ((filep=fopen(filen,"w")) == NULL) { /*Luo uuden tiedoston/tyhjentää vanhan, en saanut tehtyä tätä open() kutsulla  */
      perror("fopen");
      exit(1);
    } else {
      fclose(filep);
    }   
  }
  if (filen != NULL) { 	/*dup() lähde; https://stackoverflow.com/questions/29154056/redirect-stdout-to-a-file */
    filefd = open(filen, O_WRONLY|O_CREAT);
  }
  
  int pid;
  switch (pid = fork()) {
  case -1:          	/* fork-error */
    exit(1);
  case 0:           	/* lapsi prosessi */
    if (filen != NULL) {
      close(1);
      dup(filefd); /*output ohjataan tiedostoon */
    }
    if (execv(pathstr, argw) == -1) {
      write(STDERR_FILENO, error_message, strlen(error_message));
      perror("execv");
      exit(1);
    }
    break;
    
  default:          	/* vanhempi prosessi */
    if (wait(&pid) == -1) { 	 
      write(STDERR_FILENO, error_message, strlen(error_message));
      perror("wait");
      exit(1);
    }
    if (filen!= NULL) {
      close(filefd);
    }
    return 0;
  }
  return 0;
}
char *parsefile(char *rivi) { /*Jaetaan saatu merkkijono jos merkki ">"
				löytyy, ja palautetaan ">" jälkimmäiset merkit */
  char *arg2; /* eli tiedosto */
  arg2 = strtok(rivi,">");
  arg2 = strtok(NULL," >\n");
  return arg2;
}
char **parse(char *rivi) { /*luodaan saadusta merkkijonosta lista argumentteja,joka palautetaan*/
  char **argw = malloc(sizeof(char)*MAXKOKO);
  if (argw  == NULL) {write(STDERR_FILENO, error_message, strlen(error_message));perror("malloc");}
  char *arg;
  int i=0;
  arg = strtok(rivi,STRTOKSEP);
  while (arg != NULL) {  /*Listan luonti argumenteista */
    argw[i] = arg;
    i++;
    arg = strtok(NULL, STRTOKSEP);
  }
  argw[i] = NULL;
  return argw;
}
char *luku(void) { /*luetaan stdin, palautetaan saatu merkkijono */
  size_t koko = 0;
  char *rivi = NULL;
  if (getline(&rivi,&koko,stdin) == -1) { /* getline-funktion toiminta https://brennan.io/2015/01/16/write-a-shell-in-c/ */
    perror("getline");
  }
  return rivi;
}
char *batchluku(char *pathstr,char **argv) { /*batchtiedoston luku,parse ja suoritus */
  char *filen = NULL;
  FILE *filep;
  if ((filep = fopen(argv[1],"r")) == NULL) { /*tiedoston lukua */
    perror("fopen");
    exit(1);
  }
  char **argw = NULL;
  size_t koko = 0;
  char *rivi = NULL;
  while ((getline(&rivi,&koko,filep)) != -1) { /*tiedoston rivien läpikäyminen */
    filen = parsefile(rivi); /*tdsto nimi,parsee myös rivin sihen kuntoon,
			       että argw voi ottaa sen sisään (poistaa kaikki > merkit,ja niiden jälkeen olevat asiat) */
    argw = parse(rivi);  	/*lista argumentteja */
    pathstr = presuoritus(argw,pathstr,rivi,filen);
    rivi = malloc(sizeof(char)*MAXKOKO); /*pientä kikkailua, jotta ei muistivuotoja,
					   getline ei suostunut toimimaan ilman näitä, sillä presuorituksessa vapautetaan rivi-muuttuja*/
  }
  free(rivi); //kikkailu jatkuu
  fclose(filep);
  return pathstr;
}
