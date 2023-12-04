#include "../inc/emulator.hpp"
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>

using namespace std;

void read_hex(FILE* in){
  char * line = NULL;
  size_t len = 0;
  int adr;
  int value;
  int read;
  int b0, b1, b2, b3;
  
  while ((read = getline(&line, &len, in)) != -1 && strcmp(line, "\n")!=0){ 
    sscanf(line, "%x: %x %x %x %x\n", &adr, &b0, &b1, &b2, &b3);
    //printf("%x: %x %x %x %x\n", adr, b0 & 0xff, b1 & 0xff, b2 & 0xff, b3 & 0xff);
    memory[adr]=b0; 
    memory[adr+1]=b1; 
    memory[adr+2]=b2;
    memory[adr+3]=b3;
  }
}
void load(){
  switch(modif){
    case 0:{
      gpr[a]=csr[b-1];
      break;
    }
    case 1:{
      gpr[a]=gpr[b]+d;
      break;
    }
    case 2:{
      int val;
      if(b!=0 && d!=0) val = gpr[b]+gpr[c]+d;
      else if(d==0) val = gpr[b]+gpr[c]+d;
      else val = gpr[b]+gpr[c]+d+gpr[15];
      //0x44 33 22 11 -> 0x11223344
      gpr[a] = (memory[val] & 0xff) | (memory[val+1]<<8  & 0x0000ff00) | 
        (memory[val+2]<<16 & 0x00ff0000) | (memory[val+3]<<24 & 0xff000000);
      break;
    }
    case 3:{
      int val = gpr[b];
      gpr[a] =  (memory[val] & 0xff) | (memory[val+1]<<8  & 0x0000ff00) | 
        (memory[val+2]<<16 & 0x00ff0000) | (memory[val+3]<<24 & 0xff000000);
      if(a==0xf) gpr[a]-=4;
      gpr[b]=gpr[b]+d;
      break;
    }
    case 4:{
      csr[a-1]=gpr[b];
      break;
    }
    case 5:{
      csr[a-1]=csr[b-1]|d;
      break;
    }
    case 6:{
      int val;
      if(d!=0) val = gpr[b]+gpr[c]+d+gpr[15];
      csr[a-1]= (memory[val] & 0xff) | (memory[val+1]<<8  & 0x0000ff00) | 
        (memory[val+2]<<16 & 0x00ff0000) | (memory[val+3]<<24 & 0xff000000);
      break;
    }
    case 7:{
      int val;
      if(d!=0) val = gpr[b]+gpr[c]+d+gpr[15];
      csr[a-1]= (memory[val] & 0xff) | (memory[val+1]<<8  & 0x0000ff00) | 
        (memory[val+2]<<16 & 0x00ff0000) | (memory[val+3]<<24 & 0xff000000);
      break;
    }
  }
}
void set_time_cfg(int x){
  if(x==0){
    time_cfg=0.5;
  }
  else if(x==1){
    time_cfg=1;
  }
  else if(x==2){
    time_cfg=1.5;
  }
  else if(x==3){
    time_cfg=2;
  }
  else if(x==4){
    time_cfg=5;
  }
  else if(x==5){
    time_cfg=10;
  }
  else if(x==6){
    time_cfg=30;
  }
  else if(x==7){
    time_cfg=60;
  }
}

void store(){
  switch(modif){
    case 0:{
      int val = gpr[a]+gpr[b]+d;
      if(val==0xFFFFFF00){
        char x = (gpr[c] & 0xff);
        int v = 0x4000003c;
        int y = (memory[v] & 0xff) | (memory[v+1]<<8  & 0x0000ff00) | 
        (memory[v+2]<<16 & 0x00ff0000) | (memory[v+3]<<24 & 0xff000000);
        printf("%c %x\n",x,y);

      }
      else if(val==0xFFFFFF10){
        set_time_cfg(gpr[c]);
      }
      memory[val] = gpr[c] & 0xff;
      memory[val+1] = gpr[c]>>8 & 0xff;
      memory[val+2] = gpr[c]>>16 & 0xff;
      memory[val+3] = gpr[c]>>24 & 0xff;
      
      break;
    }
    case 2:{
      int val = gpr[a]+gpr[b]+d+gpr[15];
      int val1 = (memory[val] & 0xff) | (memory[val+1]<<8  & 0x0000ff00) | 
        (memory[val+2]<<16 & 0x00ff0000) | (memory[val+3]<<24 & 0xff000000);
      if(val1==0xFFFFFF00){
        char x = (gpr[c] & 0xff);
        int v = 0x4000003c;
        int y = (memory[v] & 0xff) | (memory[v+1]<<8  & 0x0000ff00) | 
        (memory[v+2]<<16 & 0x00ff0000) | (memory[v+3]<<24 & 0xff000000);
        printf("%c",x);
        fflush(stdout);
      }
      else if(val1==0xFFFFFF10){
        set_time_cfg(gpr[c]);
      }
      memory[val1] = gpr[c] & 0xff;
      memory[val1+1] = gpr[c]>>8 & 0xff;
      memory[val1+2] = gpr[c]>>16 & 0xff;
      memory[val1+3] = gpr[c]>>24 & 0xff;

      break;
    }
    case 1:{
      gpr[a]=gpr[a]-d;
      if(gpr[a]==0xFFFFFF00){
        char x = (gpr[c] & 0xff);
        printf("%c",x);
        fflush(stdout);
      }
      else if(gpr[a]==0xFFFFFF10){
        set_time_cfg(gpr[c]);
      }
      memory[gpr[a]] = gpr[c] & 0xff;
      memory[gpr[a]+1] = gpr[c]>>8 & 0xff;
      memory[gpr[a]+2] = gpr[c]>>16 & 0xff;
      memory[gpr[a]+3] = gpr[c]>>24 & 0xff;
      
      break;
    }
  }
}

void shift(){
  switch(modif){
    case 0:{
      gpr[a]=gpr[b]<<gpr[c];
      break;
    }
    case 1:{
      gpr[a]=gpr[b]>>gpr[c];
      break;
    } 
  }
}

void logic(){
  switch(modif){
    case 0:{
      gpr[a]=~gpr[b];
      break;
    }
    case 1:{
      gpr[a]=gpr[b]&gpr[c];
      break;
    }
    case 2:{
      gpr[a]=gpr[b]|gpr[c];
      break;
    }
    case 3:{
      gpr[a]=gpr[b]^gpr[c];
      break;
    }
  }
}

void arith(){
  switch(modif){
    case 0:{
      gpr[a]=gpr[b]+gpr[c];
      break;
    }
    case 1:{
      gpr[a]=gpr[b]-gpr[c];
      break;
    }
    case 2:{
      gpr[a]=gpr[b]*gpr[c];
      break;
    }
    case 3:{
      gpr[a]=gpr[b]/gpr[c];
      break;
    }
  }
}

void aswap(){
  int temp = gpr[b];
  gpr[b]=gpr[c];
  gpr[c]=temp;
}

void jump(){
  switch(modif){
    case 0:{
      gpr[15]=gpr[a]+d;
      break;
    }
    case 1:{
      if(gpr[b]==gpr[c])
        gpr[15]=gpr[a]+d;
      else gpr[15]+=4;
      break;
    }
    case 2:{
      if(gpr[b]!=gpr[c])
        gpr[15]=gpr[a]+d;
      else gpr[15]+=4;
      break;
    }
    case 3:{
      if(gpr[b]>gpr[c])
        gpr[15]=gpr[a]+d;
      else gpr[15]+=4;
      break;
    }
    case 8:{
      if(d>0) d=d+gpr[15];
      int val = gpr[a]+d;
      gpr[15]= (memory[val] & 0xff) | (memory[val+1]<<8  & 0x0000ff00) | 
        (memory[val+2]<<16 & 0x00ff0000) | (memory[val+3]<<24 & 0xff000000);
      break;
    }
    case 9:{
      if(gpr[b]==gpr[c]){
        if(d>0) d=d+gpr[15];
        int val = (gpr[a]+d);
        gpr[15]= (memory[val] & 0xff) | (memory[val+1]<<8  & 0x0000ff00) | 
        (memory[val+2]<<16 & 0x00ff0000) | (memory[val+3]<<24 & 0xff000000);
      }
      else gpr[15]+=4;
      break;
    }
    case 10:{

      if(gpr[b]!=gpr[c]){
        if(d>0) d=d+gpr[15];
        int val=gpr[a]+d;
        gpr[15]= (memory[val] & 0xff) | (memory[val+1]<<8  & 0x0000ff00) | 
        (memory[val+2]<<16 & 0x00ff0000) | (memory[val+3]<<24 & 0xff000000);
      }
      else gpr[15]+=4;
      break;
    }
    case 11:{
      if(gpr[b]>gpr[c]){
        if(d>0) d=d+gpr[15];
        int val = gpr[a]+d;
        gpr[15]= (memory[val] & 0xff) | (memory[val+1]<<8  & 0x0000ff00) | 
        (memory[val+2]<<16 & 0x00ff0000) | (memory[val+3]<<24 & 0xff000000);
      }
      else gpr[15]+=4;
    }
  }
}

void call(){
  switch (modif){
    case 0:{
      //push pc
      gpr[14]-=4;
      gpr[15]+=4;
      memory[gpr[14]] = gpr[15] & 0xff;
      memory[gpr[14]+1] = gpr[15]>>8 & 0xff;
      memory[gpr[14]+2] = gpr[15]>>16 & 0xff;
      memory[gpr[14]+3] = gpr[15]>>24 & 0xff;
      gpr[15]=gpr[a]+gpr[b]+d;
      break;
    }
    case 1:{
      //push pc
      if(d>0) d=d+gpr[15];
      gpr[14]-=4;
      gpr[15]+=4;
      memory[gpr[14]] = gpr[15] & 0xff;
      memory[gpr[14]+1] = gpr[15]>>8 & 0xff;
      memory[gpr[14]+2] = gpr[15]>>16 & 0xff;
      memory[gpr[14]+3] = gpr[15]>>24 & 0xff;
      int val = gpr[a]+gpr[b]+d;
      gpr[15]= (memory[val] & 0xff) | (memory[val+1]<<8  & 0x0000ff00) | 
        (memory[val+2]<<16 & 0x00ff0000) | (memory[val+3]<<24 & 0xff000000);
      break;
    }
  }
  
}

void inti(){
  gpr[15]+=4;
    if(!(csr[0]&0x1)){
    //push status
    gpr[14]-=4;
    memory[gpr[14]] = csr[0] & 0xff;
    memory[gpr[14]+1] = csr[0]>>8 & 0xff;
    memory[gpr[14]+2] = csr[0]>>16 & 0xff;
    memory[gpr[14]+3] = csr[0]>>24 & 0xff;
    //push pc
    gpr[14]-=4;
    memory[gpr[14]] = gpr[15] & 0xff;
    memory[gpr[14]+1] = gpr[15]>>8 & 0xff;
    memory[gpr[14]+2] = gpr[15]>>16 & 0xff;
    memory[gpr[14]+3] = gpr[15]>>24 & 0xff;
    
    csr[2]=4; //cause
    csr[0]=csr[0]&(~0x1);  //status
    gpr[15]=csr[1];       //handle
  }
}

void halt(){
  end_e=true;
  gpr[15]+=4;
}

void print_regs(){
  cout << "\n-----------------------------------------------------------------\n";
  cout << "Emulated processor executed halt instruction\n";
  cout << "Emulated processor state:\n";
  printf(" r0=%.8x  r1=%.8x  r2=%.8x  r3=%.8x\n",gpr[0],gpr[1],gpr[2],gpr[3]);
  printf(" r4=%.8x  r5=%.8x  r6=%.8x  r7=%.8x\n",gpr[4],gpr[5],gpr[6],gpr[7]);
  printf(" r8=%.8x  r9=%.8x r10=%.8x r11=%.8x\n",gpr[8],gpr[9],gpr[10],gpr[11]);
  printf("r12=%.8x r13=%.8x r14=%.8x r15=%.8x\n",gpr[12],gpr[13],gpr[14],gpr[15]);
   printf("status=%.8x handler=%.8x cause=%.8x \n",csr[0],csr[1],csr[2]);
}

void execute(){
  instr =  (memory[gpr[15]]>>4) & 0xf;
  modif =  memory[gpr[15]] & 0xf;
  a =  (memory[gpr[15]+1]>>4) & 0xf;
  b =  memory[gpr[15]+1] & 0xf;
  c =  (memory[gpr[15]+2]>>4) & 0xf;
  d = (memory[gpr[15]+2] & 0xf00) | (memory[gpr[15]+3] & 0xff);
  
  switch(instr){
    case 9:{
      load();
      gpr[15]+=4;
      break;
    }
    case 8:{
      store();
      gpr[15]+=4;
      break;
    }
    case 7:{
      shift();
      gpr[15]+=4;
      break;
    }
    case 6:{
      logic();
      gpr[15]+=4;
      break;
    }
    case 5:{
      arith();
      gpr[15]+=4;
      break;
    }
    case 4:{
      aswap();
      gpr[15]+=4;
      break;
    }
    case 3:{
      jump();
      break;
    }
    case 2:{
      call();
      break;
    }
    case 1:{
      inti();
      break;
    }
    case 0:{
      halt();
      break;
    }
  }
}



int main(int argc, char* argv[]){
  string in;
  string out;
  end_e = false;
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  std::clock_t start = clock();
  double duration;


  if(argc==2 && strcmp(argv[0],"./emulator")==0){ 
    in=argv[1];
    FILE* inFile = fopen(in.c_str(), "r");
    if(inFile==NULL){
      cerr << "Greska, pogresan ulazni fajl" << endl;
      return -1;
    }
    read_hex(inFile);
    
    //r14 = sp, r15 = pc
    for(int i=0; i<=15; i++) gpr.push_back(0);
    for(int i=0; i<3; i++) csr.push_back(0);
    gpr[15] = 0x40000000;
    gpr[0] = 0;
    

    while(!end_e){
      struct termios oldT, newT;
      tcgetattr(0, &oldT);
      newT = oldT;
      newT.c_lflag &= ~(ICANON | ECHO);
      tcsetattr(0, 0, &newT);

      fcntl(0, F_SETFL, O_NONBLOCK); //0=strdin, set flags: ne blokira 
      fd_set readSet;
      FD_ZERO(&readSet);  //cisti sve postavljene bite iz deskriptora readSet
      FD_SET(0, &readSet);  //0=stdin

      
      if (select(1, &readSet, NULL, NULL, &timeout) > 0) {
        char ch;
        read(0, &ch, 1);
        if (isprint(ch)) {
          char x = ch;
          memory[0xffffff04]=x;
          if(!(csr[0]&0x2)){
            gpr[14]-=4;
            memory[gpr[14]] = csr[0] & 0xff;
            memory[gpr[14]+1] = csr[0]>>8 & 0xff;
            memory[gpr[14]+2] = csr[0]>>16 & 0xff;
            memory[gpr[14]+3] = csr[0]>>24 & 0xff;
            //push pc
            gpr[14]-=4;
            memory[gpr[14]] = gpr[15] & 0xff;
            memory[gpr[14]+1] = gpr[15]>>8 & 0xff;
            memory[gpr[14]+2] = gpr[15]>>16 & 0xff;
            memory[gpr[14]+3] = gpr[15]>>24 & 0xff;
            csr[2]=3; //cause
            csr[0]=csr[0]&(~0x2);  //status
            gpr[15]=csr[1];       //handle
          }
        }
      }
      execute();
      if(((clock()-start)/(double) CLOCKS_PER_SEC)>=time_cfg){
        start = clock();
        if(!(csr[0]&0x4)){
          gpr[14]-=4;
          memory[gpr[14]] = csr[0] & 0xff;
          memory[gpr[14]+1] = csr[0]>>8 & 0xff;
          memory[gpr[14]+2] = csr[0]>>16 & 0xff;
          memory[gpr[14]+3] = csr[0]>>24 & 0xff;
          //push pc
          gpr[14]-=4;
          memory[gpr[14]] = gpr[15] & 0xff;
          memory[gpr[14]+1] = gpr[15]>>8 & 0xff;
          memory[gpr[14]+2] = gpr[15]>>16 & 0xff;
          memory[gpr[14]+3] = gpr[15]>>24 & 0xff;
          
          csr[2]=2; //cause
          csr[0]=csr[0]&(~0x4);  //status
          gpr[15]=csr[1];       //handle
        }
      }
    }
  }
  else{
    cerr << "Greska, pogresni argumenti" << endl;
    return -1;
  }
  print_regs();
  struct termios oldTerm;
  tcgetattr(0, &oldTerm);

  oldTerm.c_lflag |= (ICANON | ECHO);
  tcsetattr(0, 0, &oldTerm);

  int flags = fcntl(0, F_GETFL, 0);
  fcntl(0, F_SETFL, flags & ~O_NONBLOCK);

	return 0;
}