#include <stdio.h>
#include <cmath>
#include "../inc/assembler.hpp"

using namespace std;

extern list<Arg*> arg_list;
extern list<Instr*> instr_list;


void add_to_table(Type t, const char* name){
  
  switch(t){
    case SCTN:{
      symbols_table.push_front(
        new Symbol(symbols_table.size()+1, 0, "SCTN","LOC",name, name));
      section->size=section->counter;
      sections.push_back(section);
      section = new Section(name, symbols_table.size());
      break;
    }
    case EXT:{
      symbols_table.push_back(
        new Symbol(symbols_table.size()+1, 0, "NOTYP","GLOB", "UND", name));
      break;
    }
    case GLOB:{
      symbols_table.push_back(
        new Symbol(symbols_table.size()+1, 0, "NOTYP","GLOB", "UND", name));
      break;
    }
  }
}
void add_to_reloc(const char* symbol_name, int offset, const char* type, int add){
  int ad1, ad2;
  for(auto const& sim: symbols_table){
    if(strcmp(sim->name,symbol_name)==0){
      int ad1 = 0;
      if(strcmp(sim->bind,"GLOB")==0 || strcmp(sim->type, "SCTN")==0){
        ad1=0;
      }
      else{
        ad1=sim->value;
        relocation_table.push_back(
          new Reloc(offset, type, sim->name, ad1, add));
        return;
      }
      ad1=sim->value;
      relocation_table.push_back(
        new Reloc(offset, type, symbol_name, ad1, add));
      return;
    }
  }
}
bool is_directive(const Instr* i){
  if(strcmp(i->name, ".section")==0){
    if(i->arg.size()==1){
      add_to_table(SCTN, (*(i->arg.begin()))->name);
    }
    return true;
  }
  else if(strcmp(i->name, ".extern")==0){
    if(i->arg.size()<0){}
    else{
      for(auto const& ar: i->arg){
        add_to_table(EXT, ar->name);
      }
    }
    return true;
  }
  else if(strcmp(i->name, ".global")==0) {
    if(i->arg.size()<0){}
    else{
      for(auto const& ar: i->arg){
        bool fnd = false;
        for(Symbol* s: symbols_table){
          if(strcmp(s->name, ar->name)==0){
            s->type = "GLOB";
            fnd=true; break;
          }
        }
        if(!fnd) add_to_table(GLOB, ar->name);
      }
    }
    return true;
  }
  else if(strcmp(i->name, ".skip")==0){
    if(i->arg.size()<0){}
    else{
      for(int k=0; k<i->arg.front()->offset; k++){
        section->push_mem(0);
      }
      section->counter+=(i->arg.front()->offset)*4;
    }
    return true;
  }
  else if(strcmp(i->name, ".word")==0){
    for(auto const& ar: i->arg){
      section->counter+=4;
      section->push_mem(0);
    }
    return true;
  }
  else if(strcmp(i->name, ".ascii")==0){
    const char* x = i->arg.front()->name;
    int val=0;
    for(int i=1; i<strlen(x)-1; i++){
      char xx = x[i];
      section->counter++;
      section->section_mem.push_back(xx);
    }
    return true;
  }
  else if(strcmp(i->name, ".equ")==0){ 
  //.equ timer_config, 0xFFFFFF10
  //.equ character_offset, 2
  //.equ message_len, message_end - message_start
    char* sim = i->arg.front()->name;
    auto it = i->arg.begin();
    it++;
    Arg* val = *it;
    if(val->name){ //char*, Arg*, 
      int br;
      int f = false;
      for(Symbol* s: symbols_table){
        if(strcmp(s->name, sim)==0){
          f=true;
          s->ndx="ABS1";
          tns_table.push_back(new Tns(sim, val));
          break;
        }
      }
      if(!f){
        symbols_table.push_back(new Symbol(symbols_table.size()+1, 0, "NOTYP","LOC", "ABS1", sim));
        tns_table.push_back(new Tns(sim, val));
      }
    }
    else{
      int f = false;
      for(Symbol* s: symbols_table){
        if(strcmp(s->name, sim)==0){
          f=true;
          s->ndx="ABS";
          s->value=val->offset;
          break;
        }
      }
      if(!f){
        symbols_table.push_back(new Symbol(symbols_table.size()+1, val->offset, "NOTYP","LOC", "ABS", sim));
      }
      
    }
    return true;
  }
  else if(strcmp(i->name, ".end")==0){
      section->size=section->counter;
      sections.push_back(section);
      return true;
  }
  return false;
}
int reg_to_int(const char* reg){ //reg = %r1
  if(strcmp(reg,"%sp")==0) return 14;
  else if(strcmp(reg,"%pc")==0) return 15;
  string r = reg;
  return stoi(r.substr(2));
}
bool is_instruction1(const Instr* i){
  if(strcmp(i->name, "halt")==0){ 
    section->counter+=4;
    section->push_mem(0);
  }
  else if(strcmp(i->name, "int")==0){ 
    section->counter+=4;
    section->push_mem(0x10000000);  
  }
  else if(strcmp(i->name, "iret")==0){ //pop pc -> pc=mem32[sp]; sp=sp+4
                                       //pop status -> status=mem32[sp]; sp=sp+4
    section->counter+=8;
    section->push_mem(0x97000000 | 1<<20 | 14<<16 | 0x0); 
    section->push_mem(0x93000000 | 15<<20 | 14<<16 | 0x8); 
  }
  else if(strcmp(i->name, "ret")==0){ //pop pc -> pc=mem32[sp]; sp=sp+4
    section->counter+=4;
    section->push_mem(0x93000000 | 15<<20 | 14<<16 | 0x4); 
  }
  else if(strcmp(i->name, "call")==0){ // call operand (push pc; pc<=operand)
    section->counter+=4;
    Arg* a = (i->arg.front());
    if(a->offset){
      if(a->offset < pow(2,12) && a->offset>=0){
        int s = 0x20000000;
        s = s | a->offset;
        section->push_mem(s);
      }
      else{
        section->push_mem(0x21000000);
      }
    }
    else if(a->name){
      section->push_mem(0x21000000);
    }
  }
  else if(strcmp(i->name, "jmp")==0){ 
    section->counter+=4;
    Arg* a = (i->arg.front());
    if(a->offset){
      if(a->offset < pow(2,12) && a->offset>=0){
        int s = 0x30000000;
        s = s | a->offset;
        section->push_mem(s);
      }
      else{
        section->push_mem(0x38000000);
      }
    }
    else{
      section->push_mem(0x38000000);
    }
  }
  else if(strcmp(i->name, "beq")==0){ //%gpr1, %gpr2, operand (==)
    section->counter+=4;
    auto it = i->arg.begin();
    int gpr1 = reg_to_int((*it)->name);
    it++;
    int gpr2 = reg_to_int((*it)->name);
    it++;
    Arg* a = *it;
    if(a->offset){
      if(a->offset < pow(2,12) && a->offset>=0){
        int s = 0x31000000;
        s = s | a->offset;
        section->push_mem(s | gpr1<<16 | gpr2<<12);
      }
      else{
        section->push_mem(0x39000000 | gpr1<<16 | gpr2<<12);
      }
    }
    else if(a->name) {
      section->push_mem(0x39000000 | gpr1<<16 | gpr2<<12);
    }
  }
  else if(strcmp(i->name, "bne")==0){ //%gpr1, %gpr2, operand (!=)
    section->counter+=4;
    auto it = i->arg.begin();
    int gpr1 = reg_to_int((*it)->name);
    it++;
    int gpr2 = reg_to_int((*it)->name);
    it++;
    Arg* a = *it;
    if(a->offset){
      if(a->offset < pow(2,12) && a->offset>=0){
        int s = 0x32000000;
        s = s | a->offset;
        section->push_mem(s | gpr1<<16 | gpr2<<12);
      }
      else{
        section->push_mem(0x3A000000 | gpr1<<16 | gpr2<<12);
      }
    }
    else if(a->name) {
      section->push_mem(0x3A000000 | gpr1<<16 | gpr2<<12);
    }
  }
  else if(strcmp(i->name, "bgt")==0){ //%gpr1, %gpr2, operand (>)
    section->counter+=4;
    auto it = i->arg.begin();
    int gpr1 = reg_to_int((*it)->name);
    it++;
    int gpr2 = reg_to_int((*it)->name);
    it++;
    Arg* a = *it;
    if(a->offset){
      if(a->offset < pow(2,12) && a->offset>=0){
        int s = 0x33000000;
        s = s | a->offset;
        section->push_mem(s | gpr1<<16 | gpr2<<12);
      }
      else{
        section->push_mem(0x3B000000 | gpr1<<16 | gpr2<<12);
      }
    }
    else if(a->name) {
      section->push_mem(0x3B000000 | gpr1<<16 | gpr2<<12);
    }
  }
  else if(strcmp(i->name, "push")==0){  //push %gpr -> sp=sp-4; mem32[sp]=gpr
    section->counter+=4;
    auto it = i->arg.begin();
    int gpr = reg_to_int((*it)->name); //gpr[A]=sp, gpr[C]=gpr, D=0x4 (-)
    section->push_mem(0x81000000 | 14<<20 | gpr<<12 | 0x4); 
  }
  else if(strcmp(i->name, "pop")==0){  //pop %gpr -> grp=mem32[sp]; sp=sp+4
    section->counter+=4;
    auto it = i->arg.begin();
    int gpr = reg_to_int((*it)->name); //gpr[A]=gpr, gpr[B]=sp, D=0x4
    section->push_mem(0x93000000 | gpr<<20 | 14<<16 | 0x4); 
  }
  else if(strcmp(i->name, "xchg")==0){ //%gprS, %gprD
    section->counter+=4;
    auto it = i->arg.begin();
    int gpr1 = reg_to_int((*it)->name);
    it++;
    int gpr2 = reg_to_int((*it)->name);
    section->push_mem(0x40000000 | gpr2<<16 | gpr1<<12);
  }
  else if(strcmp(i->name, "add")==0){ //%gprS, %gprD
    section->counter+=4;
    auto it = i->arg.begin();
    int gpr1 = reg_to_int((*it)->name);
    it++;
    int gpr2 = reg_to_int((*it)->name);
    section->push_mem(0x50000000 | gpr2<<20 | gpr2<<16 | gpr1<<12);
  }
  else if(strcmp(i->name, "sub")==0){ 
    section->counter+=4;
    auto it = i->arg.begin();
    int gpr1 = reg_to_int((*it)->name);
    it++;
    int gpr2 = reg_to_int((*it)->name);
    section->push_mem(0x51000000 | gpr2<<20 | gpr2<<16 | gpr1<<12);
  }
  else if(strcmp(i->name, "mul")==0){ 
    section->counter+=4;
    auto it = i->arg.begin();
    int gpr1 = reg_to_int((*it)->name);
    it++;
    int gpr2 = reg_to_int((*it)->name);
    section->push_mem(0x52000000 | gpr2<<20 | gpr2<<16 | gpr1<<12);
  }
  else if(strcmp(i->name, "div")==0){ 
    section->counter+=4;
    auto it = i->arg.begin();
    int gpr1 = reg_to_int((*it)->name);
    it++;
    int gpr2 = reg_to_int((*it)->name);
    section->push_mem(0x53000000 | gpr2<<20 | gpr2<<16 | gpr1<<12);
  }
  else if(strcmp(i->name, "not")==0){ 
    section->counter+=4;
    auto it = i->arg.begin();
    int gpr1 = reg_to_int((*it)->name);
    it++;
    int gpr2 = reg_to_int((*it)->name);
    section->push_mem(0x60000000 | gpr2<<20 | gpr2<<16 | gpr1<<12);
  }
  else if(strcmp(i->name, "and")==0){ 
    section->counter+=4;
    auto it = i->arg.begin();
    int gpr1 = reg_to_int((*it)->name);
    it++;
    int gpr2 = reg_to_int((*it)->name);
    section->push_mem(0x61000000 | gpr2<<20 | gpr2<<16 | gpr1<<12);
  }
  else if(strcmp(i->name, "or")==0){ 
    section->counter+=4;
    auto it = i->arg.begin();
    int gpr1 = reg_to_int((*it)->name);
    it++;
    int gpr2 = reg_to_int((*it)->name);
    section->push_mem(0x62000000 | gpr2<<20 | gpr2<<16 | gpr1<<12);
  }
  else if(strcmp(i->name, "xor")==0){
    section->counter+=4;
    auto it = i->arg.begin();
    int gpr1 = reg_to_int((*it)->name);
    it++;
    int gpr2 = reg_to_int((*it)->name);
    section->push_mem(0x63000000 | gpr2<<20 | gpr2<<16 | gpr1<<12);
  }
  else if(strcmp(i->name, "shl")==0){
    section->counter+=4;
    auto it = i->arg.begin();
    int gpr1 = reg_to_int((*it)->name); //gprS
    it++;
    int gpr2 = reg_to_int((*it)->name); //gprD
    section->push_mem(0x70000000 | gpr2<<20 | gpr2<<16 | gpr1<<12);
  }
  else if(strcmp(i->name, "shr")==0){ 
    section->counter+=4;
    auto it = i->arg.begin();
    int gpr1 = reg_to_int((*it)->name); //gprS
    it++;
    int gpr2 = reg_to_int((*it)->name); //gprD
    section->push_mem(0x71000000 | gpr2<<20 | gpr2<<16 | gpr1<<12);
  }
  else if(strcmp(i->name, "ld")==0){  //ld operand, %gpr
    section->counter+=4;
    auto it = i->arg.begin();
    Arg* a = *it;
    it++;
    int gpr1 = reg_to_int((*it)->name);
    if(a->name && a->offset){ //[%reg+0x1]    -> mem[reg+0x1] !
      int gpr2 = reg_to_int(a->name);
      section->push_mem(0x92000000 | gpr1<<20 | gpr2<<16 | a->offset); 
    }
    else if(a->name && a->name2){// r1 = [reg+simbol]
      int gpr2 = reg_to_int(a->name);
      section->push_mem(0x92000000 | gpr1<<20 | gpr2<<16);  //dodati u relok
    }
    else if(a->name){
      if(a->name[0]=='$'){ //vrednost simbola ili literala 
        if(a->name[1]>='0' && a->name[1]<='9'){ //literal
          string na = a->name;
          if(na.size()>3 && a->name[1]=='0' && a->name[2]=='x'){ //heksa vrednost
            if(na.size()-3<=3){
              na=na.substr(3);
              section->push_mem(0x91000000 | gpr1<<20 | stoi(na, 0, 16));
            }
            else { //ne staje u 12D
              section->push_mem(0x92000000 | gpr1<<20);
            }
          }
          else{ //decimalna vrednost
            na=na.substr(1);
            int offset = stoi(na);
            if(offset < pow(2,12) && offset>=0){
              section->push_mem(0x91000000 | gpr1<<20 | offset);
            }
            else { //ne staje u 12D
              section->push_mem(0x92000000 | gpr1<<20);
            }
          }
        }
        else{ //simbol -> vrednost simbola
          section->push_mem(0x92000000 | gpr1<<20); //dodati relok
        }
      }
      else if(a->name[0]=='%'){ //%r1    -> r1
        if(a->name[1]=='r'){
          int gpr2 = reg_to_int(a->name);
          section->push_mem(0x91000000 | gpr1<<20 | gpr2<<16);
        }
      }
      else if(a->name[0]=='['){ 
        string aa = a->name;
        if(a->name[2]=='r'){ //gpr = mem[reg]
          aa=aa.substr(3,aa.size()-3);
          section->push_mem(0x92000000 | gpr1<<20 | stoi(aa)<<16);
        }
      }
      else{ //vr iz mem na adr simbol
        //gpr[a]=mem[d]     
        //gpr[A]=mem[gpr[a]]
        section->push_mem(0x92000000 | gpr1<<20);
        section->counter+=4;
        section->push_mem(0x92000000 | gpr1<<20 | gpr1<<16);
      }
    }
    else if(a->offset){//0x1     -> mem[0x1]
      if(a->offset < pow(2,12) && a->offset>=0){
        section->push_mem(0x92000000 | gpr1<<20 | a->offset);
      }
      else{
        section->push_mem(0x92000000 | gpr1<<20);
        section->counter+=4;
        section->push_mem(0x92000000 | gpr1<<20 | gpr1<<16);
      }
    }
  }
  else if(strcmp(i->name, "st")==0){  //st %gpr, operand
                                      //operand <= gpr
    section->counter+=4;
    auto it = i->arg.begin();
    int gpr1 = reg_to_int((*it)->name);
    it++;
    Arg* a = *it;
    if(a->name && a->offset){ //[%r1+0x1] -> mem[r1+0x1]=gpr
      int gpr2 = reg_to_int(a->name);
      section->push_mem(0x80000000 | a->offset | gpr1<<12 | gpr2<<20);
    }
    else if(a->name && a->name2){ //[%r1+a]   -> mem[r1+a]=gpr
      int gpr2 = reg_to_int(a->name);
      section->push_mem(0x80000000 | gpr1<<12 | gpr2<<20); //dodati relok
    }
    else if(a->offset){ //0x1 -> mem[0x1]=gpr       80000000
      if(a->offset<pow(2,12) && a->offset>=0){
        section->push_mem(0x80000000 | gpr1<<12 | a->offset);
      }
      else{
        section->push_mem(0x82000000 | gpr1<<12); //bazen
      }
    }
    else if(a->name){
      if(a->name[0]=='%'){ //%r1 -> r1=gpr
        if(a->name[1]=='r'){
          section->push_mem(0x91000000 | gpr1<<20 | reg_to_int(a->name)<<16);
        }
        else if(strcmp(a->name,"%status")==0){ //status=gpr
          section->push_mem(0x94000000 | gpr1<<16 | 0x1<<20);
        }
        else if(strcmp(a->name, "%handler")==0){
          section->push_mem(0x94000000 | gpr1<<16 | 0x2<<20);
        }
        else if(strcmp(a->name, "%cause")==0){
          section->push_mem(0x94000000 | gpr1<<16 | 0x3<<20);
        }
      }       
      else if(a->name[0]=='['){ //[%r1] -> mem[r1]=gpr
        string aa = a->name;
        aa = aa.substr(3, aa.size()-3);
        section->push_mem(0x80000000 | gpr1<<12 | stoi(aa)<<20);
      }
      else{ //a   -> mem[a]=gpr  
        section->push_mem(0x82000000 | gpr1<<12); //bazen+relok
      }
    }
  }
  else if(strcmp(i->name, "csrrd")==0){ //csrrd %csr, %gpr (gpr = csr)
    section->counter+=4;
    auto it = i->arg.begin();
    int csr = 0;
    if(strcmp((*it)->name,"%status")==0) csr=1;
    else if(strcmp((*it)->name,"%handler")==0) csr=2;
    else if(strcmp((*it)->name,"%cause")==0) csr=3;
    it++;
    int gpr = reg_to_int((*it)->name);
    section->push_mem(0x90000000 | gpr<<20 | csr<<16);
  }
  else if(strcmp(i->name, "csrwr")==0){  //csrwr %gpr, %csr (csr = gpr)
    section->counter+=4;
    auto it = i->arg.begin();
    int gpr = reg_to_int((*it)->name);
    it++;
    int csr = 0;
    if(strcmp((*it)->name,"%status")==0) csr=1;
    else if(strcmp((*it)->name,"%handler")==0) csr=2;
    else if(strcmp((*it)->name,"%cause")==0) csr=3;
    section->push_mem(0x94000000 | csr<<20 | gpr<<16);
  }
  return false;
}
bool is_directive2(const Instr* i){
  if(strcmp(i->name, ".section")==0){
    if(i->arg.size()==1){
      for(auto it = sections.begin(); it!=sections.end(); ++it){
        if((*it)->name==section->name){
          ++it;
          section = *it;
          section->counter = 0;
          break;
        }
      }
    }
  }
  else if(strcmp(i->name, ".word")==0){
    for(auto const& ar: i->arg){
      if(ar->name){
        add_to_reloc(ar->name, section->counter, "32", 4);
      }
      section->counter+=4;
    }
  }
  else if(strcmp(i->name, ".ascii")==0){
    string x = i->arg.front()->name;
    section->counter+= (int)((x.size()-2));
  }
  else if(strcmp(i->name, ".skip")==0){
    if(i->arg.size()<0){}
    else{
      section->counter+=(i->arg.front()->offset)*4;
    }
  }
  return false;
}
bool is_instruction2(const Instr* i){
  if(strcmp(i->name, "halt")==0){ 
    section->counter+=4;
  }
  else if(strcmp(i->name, "int")==0){ 
    section->counter+=4;
  }
  else if(strcmp(i->name, "iret")==0){ 
    section->counter+=8;
  }
  else if(strcmp(i->name, "call")==0){ 
    Arg* a = (i->arg.front());
    if(a->offset){
      if(a->offset>=pow(2,12) || a->offset<0){
        auto it = section->section_mem.begin();
        advance(it, section->counter);
        int val = (section->size-section->counter);
        *it = *it | (val>>24 & 0xff); ++it;
        *it = *it | (val>>16 & 0xff); ++it;
        *it = *it | (val>>8 & 0xff); ++it;
        *it = *it | (val & 0xff);
        
        int new_o = ((a->offset>>24)&0xff) | ((a->offset<<8)&0xff0000) | ((a->offset>>8)&0xff00) | ((a->offset<<24)&0xff000000);
        section->push_mem(new_o);
        section->size+=4;
      }
    }
    else if(a->name){
      auto it = section->section_mem.begin();
      advance(it, section->counter);
      int val = (section->size-section->counter);
      *it = *it | (val>>24 & 0xff); ++it;
      *it = *it | (val>>16 & 0xff); ++it;
      *it = *it | (val>>8 & 0xff); ++it;
      *it = *it | (val & 0xff);
      add_to_reloc(a->name, section->counter, section->name,4);
      section->push_mem(0);
      section->size+=4;
    }
    section->counter+=4;
  }
   else if(strcmp(i->name, "ret")==0){ 
    section->counter+=4;
  }
  else if(strcmp(i->name, "jmp")==0){ 
    Arg* a = (i->arg.front());
    if(a->offset){
      if(a->offset>=pow(2,12) || a->offset<0){
        printf("%x\n",a->offset);
        auto it = section->section_mem.begin();
        advance(it, section->counter);
        int val = (section->size-section->counter);
        *it = *it | (val>>24 & 0xff); ++it;
        *it = *it | (val>>16 & 0xff); ++it;
        *it = *it | (val>>8 & 0xff); ++it;
        *it = *it | (val & 0xff);
        int new_o = ((a->offset>>24)&0xff) | ((a->offset<<8)&0xff0000) | ((a->offset>>8)&0xff00) | ((a->offset<<24)&0xff000000);
        section->push_mem(new_o);
        section->size+=4;
      }
    }
    else if(a->name){
      auto it = section->section_mem.begin();
      advance(it, section->counter);
      int val = (section->size-section->counter);
      *it = *it | (val>>24 & 0xff); ++it;
      *it = *it | (val>>16 & 0xff); ++it;
      *it = *it | (val>>8 & 0xff); ++it;
      *it = *it | (val & 0xff);
      add_to_reloc(a->name, section->counter, section->name,4);
      section->push_mem(0);
      section->size+=4;
    }
    section->counter+=4;
  }
  else if(strcmp(i->name, "beq")==0){  //%gpr1, %gpr2, operand (==)
    auto it = i->arg.begin();
    it++; it++;
    Arg* a = *it;
    if(a->offset){
      if(a->offset>=pow(2,12) || a->offset<0){
        auto it = section->section_mem.begin();
        advance(it, section->counter);
        int val = (section->size-section->counter);
        *it = *it | (val>>24 & 0xff); ++it;
        *it = *it | (val>>16 & 0xff); ++it;
        *it = *it | (val>>8 & 0xff); ++it;
        *it = *it | (val & 0xff);
        int new_o = ((a->offset>>24)&0xff) | ((a->offset<<8)&0xff0000) | ((a->offset>>8)&0xff00) | ((a->offset<<24)&0xff000000);
        section->push_mem(new_o);
        section->size+=4;
      }
    }
    else if(a->name){
      auto it = section->section_mem.begin();
      advance(it, section->counter);
      int val = (section->size-section->counter);
      *it = *it | (val>>24 & 0xff); ++it;
      *it = *it | (val>>16 & 0xff); ++it;
      *it = *it | (val>>8 & 0xff); ++it;
      *it = *it | (val & 0xff);
      add_to_reloc(a->name, section->counter, section->name,4);
      section->push_mem(0);
      section->size+=4;
    }
    section->counter+=4;
  }
  else if(strcmp(i->name, "bne")==0){ 
    auto it = i->arg.begin();
    it++; it++;
    Arg* a = *it;
    if(a->offset){
      if(a->offset>=pow(2,12) || a->offset<0){
        auto it = section->section_mem.begin();
        advance(it, section->counter);
        int val = (section->size-section->counter);
        *it = *it | (val>>24 & 0xff); ++it;
        *it = *it | (val>>16 & 0xff); ++it;
        *it = *it | (val>>8 & 0xff); ++it;
        *it = *it | (val & 0xff);
        int new_o = ((a->offset>>24)&0xff) | ((a->offset<<8)&0xff0000) | ((a->offset>>8)&0xff00) | ((a->offset<<24)&0xff000000);
        section->push_mem(new_o);
        section->size+=4;
      }
    }
    else if(a->name){
      auto it = section->section_mem.begin();
      advance(it, section->counter);
      int val = (section->size-section->counter);
      *it = *it | (val>>24 & 0xff); ++it;
      *it = *it | (val>>16 & 0xff); ++it;
      *it = *it | (val>>8 & 0xff); ++it;
      *it = *it | (val & 0xff);
      add_to_reloc(a->name, section->counter, section->name,4);
      section->push_mem(0);
      section->size+=4;
    }
    section->counter+=4;
  }
  else if(strcmp(i->name, "bgt")==0){ 
    auto it = i->arg.begin();
    it++; it++;
    Arg* a = *it;
    if(a->offset){
      if(a->offset>=pow(2,12) || a->offset<0){
        auto it = section->section_mem.begin();
        advance(it, section->counter);
        int val = (section->size-section->counter);
        *it = *it | (val>>24 & 0xff); ++it;
        *it = *it | (val>>16 & 0xff); ++it;
        *it = *it | (val>>8 & 0xff); ++it;
        *it = *it | (val & 0xff);
        int new_o = ((a->offset>>24)&0xff) | ((a->offset<<8)&0xff0000) | ((a->offset>>8)&0xff00) | ((a->offset<<24)&0xff000000);
        section->push_mem(new_o);
        section->size+=4;
      }
    }
    else if(a->name){
      auto it = section->section_mem.begin();
      advance(it, section->counter);
      *it = *it | (section->size-section->counter);
      int val = (section->size-section->counter);
      *it = *it | (val>>24 & 0xff); ++it;
      *it = *it | (val>>16 & 0xff); ++it;
      *it = *it | (val>>8 & 0xff); ++it;
      *it = *it | (val & 0xff);
      add_to_reloc(a->name, section->counter, section->name, 4);
      section->push_mem(0);
      section->size+=4;
    }
    section->counter+=4;
  }
  else if(strcmp(i->name, "push")==0){ 
    section->counter+=4;
  }
  else if(strcmp(i->name, "pop")==0){ 
    section->counter+=4;
  }
  else if(strcmp(i->name, "xchg")==0){ //%gprS, %gprD
    section->counter+=4;
  }
  else if(strcmp(i->name, "add")==0){ 
    section->counter+=4;
  }
  else if(strcmp(i->name, "sub")==0){ 
    section->counter+=4;
  }
  else if(strcmp(i->name, "mul")==0){ 
    section->counter+=4;
  }
  else if(strcmp(i->name, "div")==0){ 
    section->counter+=4;
  }
  else if(strcmp(i->name, "not")==0){ 
    section->counter+=4;
  }
  else if(strcmp(i->name, "and")==0){ 
    section->counter+=4;
  }
  else if(strcmp(i->name, "or")==0){ 
    section->counter+=4;
  }
  else if(strcmp(i->name, "xor")==0){
    section->counter+=4;
  }
  else if(strcmp(i->name, "shl")==0){
    section->counter+=4;
  }
  else if(strcmp(i->name, "shr")==0){ 
    section->counter+=4;
  }
  else if(strcmp(i->name, "ld")==0){
    auto it = i->arg.begin();
    Arg* a = *it;
    if(a->name && a->offset){
      if(a->offset>pow(2,12) || a->offset<0) {
        cerr << "Greska, ofset: ";
        cerr<<hex<<a->offset;
        cerr<<" preveliki" << endl;
        exit(-1);
      }
    }
    else if(a->name && a->name2){
      bool found = false;
      int val;
      for(auto const& sim: symbols_table){
        if(strcmp(a->name2, sim->name)==0){
          if(sim->value>pow(2,12) || sim->value<0) {
            cerr << "Greska, simbol: "<<sim->name<<" preveliki" << endl;
            exit(-1);
          }
          val = sim->value;
          found = true;
          break;
        }
      }
      if(found==false) {
        cerr << "Greska, simbol: "<<a->name2<<" nije definisan" << endl;
        exit(-1);
      }
      else{
        auto it = section->section_mem.begin();
        advance(it, section->counter);
        *it = *it | (val>>24 & 0xff); ++it;
        *it = *it | (val>>16 & 0xff); ++it;
        *it = *it | (val>>8 & 0xff); ++it;
        *it = *it | (val & 0xff);
        //add_to_reloc(a->name2, section->counter, section->name, 1); //a->name = operand (simbol)
      }
    }
    else if(a->name){
      if(a->name[0]=='$'){ //vrednost simbola ili literala 
        if(a->name[1]>='0' && a->name[1]<='9'){ //literal
          string na = a->name;
          if(na.size()>3 && a->name[1]=='0' && a->name[2]=='x'){ //heksa vrednost
            if(na.size()-3>3){ //heksa vrednost ne staje -> bazen 
              auto it = section->section_mem.begin();
              advance(it, section->counter);
              int val = (section->size-section->counter);
              *it = *it | (val>>24 & 0xff); ++it;
              *it = *it | (val>>16 & 0xff); ++it;
              *it = *it | (val>>8 & 0xff); ++it;
              *it = *it | (val & 0xff);
              section->size+=4;
              int m=8;
              if(na.size()-3==8){
                long offset = stol(na.substr(1), nullptr, 16);
                int new_o = ((offset>>24)&0xff) | ((offset<<8)&0xff0000) | ((offset>>8)&0xff00) | ((offset<<24)&0xff000000);
                section->push_mem(new_o);
              }
              else {
                long offset = stoi(na.substr(3),0,16);
                int new_o = ((offset>>24)&0xff) | ((offset<<8)&0xff0000) | ((offset>>8)&0xff00) | ((offset<<24)&0xff000000);
                section->push_mem(new_o);
              }
            }
          }
          else{ //decimalna vrednost
            na=na.substr(1);
            int offset = stoi(na);
            if(offset >= pow(2,12) || offset<0){
              auto it = section->section_mem.begin();
              advance(it, section->counter);
              int val = (section->size-section->counter);
              *it = *it | (val>>24 & 0xff); ++it;
              *it = *it | (val>>16 & 0xff); ++it;
              *it = *it | (val>>8 & 0xff); ++it;
              *it = *it | (val & 0xff);
              int new_o = ((a->offset>>24)&0xff) | ((a->offset<<8)&0xff0000) | ((a->offset>>8)&0xff00) | ((a->offset<<24)&0xff000000);
              section->push_mem(new_o);
              section->size+=4;
            }
          }
        }
        else{ //$simbol -> vrednost simbola 
          int sl = strlen(a->name);
          char* nov = new char[sl];
          strncpy(nov, a->name+1, sl-1);
          nov[sl-1]='\0';
          add_to_reloc(nov, section->counter, section->name, 4);
          auto it = section->section_mem.begin();
          advance(it, section->counter);
          int val = (section->size-section->counter);
          *it = *it | (val>>24 & 0xff); ++it;
          *it = *it | (val>>16 & 0xff); ++it;
          *it = *it | (val>>8 & 0xff); ++it;
          *it = *it | (val & 0xff);
          section->push_mem(0);
          section->size+=4;
        }
      }
      else if(a->name[0]=='%'){ } //%r1    -> r1
      else if(a->name[0]=='['){ }
      else{ //simbol -> vrednost iz mem na adr simbol 
        add_to_reloc(a->name, section->counter, section->name, 4);
        auto it = section->section_mem.begin();
        advance(it, section->counter);
        int val = (section->size-section->counter);
        *it = *it | (val>>24 & 0xff); ++it;
        *it = *it | (val>>16 & 0xff); ++it;
        *it = *it | (val>>8 & 0xff); ++it;
        *it = *it | (val & 0xff);
        section->push_mem(0);
        section->size+=4;
        section->counter+=4;
      }
    }
    else if(a->offset){//0x1     -> mem[0x1]
      if(a->offset >= pow(2,12) || a->offset<0){
        auto it = section->section_mem.begin();
        advance(it, section->counter);
        int val = (section->size-section->counter);
        *it = *it | (val>>24 & 0xff); ++it;
        *it = *it | (val>>16 & 0xff); ++it;
        *it = *it | (val>>8 & 0xff); ++it;
        *it = *it | (val & 0xff);
        int new_o = ((a->offset>>24)&0xff) | ((a->offset<<8)&0xff0000) | ((a->offset>>8)&0xff00) | ((a->offset<<24)&0xff000000);
        section->push_mem(new_o);
        section->size+=4;
        section->counter+=4;
      }
    }
    section->counter+=4;
  }
////////////////////////////////////////////////////////////////////////////////////////////
  else if(strcmp(i->name, "st")==0){
    auto it = i->arg.begin();
    int gpr1 = reg_to_int((*it)->name);
    it++;
    Arg* a = *it;
    if(a->name && a->offset){ //[%r1+0x1] -> mem[r1+0x1]=gpr
      if(a->offset>pow(2,12) || a->offset<0) {
        cerr << "Greska, ofset preveliki" << endl;
        exit(-1);
      }
    }
    else if(a->name && a->name2){ //[%r1+a]   -> mem[r1+a]=gpr
      int gpr2 = reg_to_int(a->name);
      bool found = false;
      int val;
      for(auto const& sim: symbols_table){
        if(strcmp(a->name2, sim->name)==0){
          if(sim->value>pow(2,12) || sim->value<0) {
            cerr << "Greska, simbol preveliki" << endl;
            exit(-1);
          }
          val = sim->value;
          found = true;
          break;
        }
      }
      if(found==false) {
        cerr << "Greska, simbol: "<<a->name2<<"nije definisan" << endl;
        exit(-1);
      }
      else{
        auto it = section->section_mem.begin();
        advance(it, section->counter);
        *it = *it | (val>>24 & 0xff); ++it;
        *it = *it | (val>>16 & 0xff); ++it;
        *it = *it | (val>>8 & 0xff); ++it;
        *it = *it | (val & 0xff);
        //add_to_reloc(a->name2, section->counter, section->name, 1); //a->name = operand (simbol)//dodati relok
      }
    }
    else if(a->offset){ //0x1 -> mem[0x1]=gpr       80000000
      if(a->offset<pow(2,12) && a->offset>=0){}
      else{ //bazen
        auto it = section->section_mem.begin();
        advance(it, section->counter);
        int val = (section->size-section->counter);
        *it = *it | (val>>24 & 0xff); ++it;
        *it = *it | (val>>16 & 0xff); ++it;
        *it = *it | (val>>8 & 0xff); ++it;
        *it = *it | (val & 0xff);
        int new_o = ((a->offset>>24)&0xff) | ((a->offset<<8)&0xff0000) | ((a->offset>>8)&0xff00) | ((a->offset<<24)&0xff000000);
        section->push_mem(new_o);
        section->size+=4;
      }
    }
    else if(a->name){
      if(a->name[0]=='%'){ }       
      else if(a->name[0]=='['){ }
      else{ //a   -> mem[a]=gpr 
        auto it = section->section_mem.begin();
        advance(it, section->counter);
        int val = (section->size-section->counter);
        *it = *it | (val>>24 & 0xff); ++it;
        *it = *it | (val>>16 & 0xff); ++it;
        *it = *it | (val>>8 & 0xff); ++it;
        *it = *it | (val & 0xff);
        //0x11223344 -> 0x44332211
        int new_o = ((a->offset>>24)&0xff) | ((a->offset>>8)&0xff00) | ((a->offset<<8)&0xff0000) | ((a->offset<<24)&0xff000000);
        section->push_mem(new_o);
        section->size+=4; 
        add_to_reloc(a->name, section->counter, section->name, 4); //bazen+relok
      }
    }
    section->counter+=4;
  }
  else if(strcmp(i->name, "csrrd")==0){
    section->counter+=4;
  }
  else if(strcmp(i->name, "csrwr")==0){ 
    section->counter+=4;
  }

  return false;
}

bool is_label(const char* i){
  string ii = i;
  if(ii[ii.length()-1]==':'){
    ii.pop_back();
    bool in_list = false;
    int sl = strlen(i);
    char* nov = new char[sl];
    strncpy(nov, i, sl-1);
    nov[sl-1]='\0';
    for(auto it = symbols_table.begin(); it!=symbols_table.end(); ++it){
      string x = (*it)->name;
      if(x.compare(ii)==0){
        in_list=true;
        (*it)->value=section->counter;
        (*it)->ndx=section->name;
      }
    }
    if(in_list==false){
      int sl = strlen(i);
      char* nov = new char[sl];
      strncpy(nov, i, sl-1);
      nov[sl-1]='\0';
      symbols_table.push_back(
        new Symbol(symbols_table.size()+1, section->counter, "NOTYP", "LOC", section->name, nov));
    }
  }
  return false;
}
void pass1(){
  section = new Section("",0);
  for(auto const& i: instr_list){
    if(is_directive(i)){
    }
    else if(is_label(i->name)){
    }
    else if(is_instruction1(i)){
    }
  }
}

void middle_pass(){
  auto i = tns_table.begin();
  while(!tns_table.empty()){
    if(i==tns_table.end())
      i=tns_table.begin();
    if((*i)->arg->name && (*i)->arg->name2){
      int val1, val2; bool f1=false, f2=false;
      for(Symbol* s: symbols_table){
        if(strcmp(s->name, (*i)->arg->name)==0 && strcmp(s->ndx,"ABS1")!=0){
          f1=true; val1=s->value; 
        }
        if(strcmp(s->name, (*i)->arg->name2)==0 && strcmp(s->ndx,"ABS1")!=0){
          f2=true; val2=s->value;
        }
      }
      if(f1 && f2){
        for(Symbol* sim: symbols_table){
          if(strcmp(sim->name, (*i)->name)==0){   
            sim->ndx="ABS";  
            if((*i)->arg->op==0){
              sim->value=val1-val2; 
            }
            else sim->value=val1+val2;
            tns_table.erase(i);
          }
        }
      }
      else i++;
    }
    else{
      int val; bool f=false;
      for(Symbol* s: symbols_table){
        if(strcmp(s->name, (*i)->arg->name)==0 && strcmp(s->ndx,"ABS1")!=0){
          f=true; val=s->value; break;
        }
      }
      if(f){
        for(Symbol* sim: symbols_table){
          if(strcmp(sim->name, (*i)->name)==0){ 
            sim->value=val;
            sim->ndx="ABS";
            tns_table.erase(i);
            break;
          }
        }
      }
      else i++;
    }
  }
}

void pass2(){
  section = sections.front();
  section->counter=0;
  for(auto const& i: instr_list){
    if(is_directive2(i)){

    }
    else if(is_instruction2(i)){

    }
  }
}

void print_into_file(FILE* out){
  fprintf(out, "--> TABELA SIMBOLA <--\n");
  for(auto const& i: symbols_table){
    if(strcmp(i->type,"SCTN")==0){
      for(auto const& s: sections){
        if(strcmp(s->name, i->name)==0) {
          i->size=s->section_mem.size();
          break;
        }
      }
    }
    fprintf(out, "%d %d %d %s %s %s %s\n", i->num, i->value, i->size, i->type, i->bind, i->ndx, i->name);
  }
  fprintf(out, "\n--> RELOKACIJE <--\n");
  for(auto const& i: relocation_table){
    fprintf(out, "%d %s %s %d %d\n", i->offset, i->type, i->symbol_name, i->addend, i->addend2);
  }
  fprintf(out, "\n--> SEKCIJE <--");
  for(auto const& i: sections){
    if(i->section_mem.size()>0){
      fprintf(out, "\n%s\n",i->name);
      int br=0;
      for(auto const& m: i->section_mem){ 
        fprintf(out, "%.2x ", (unsigned int)(m & 0xFF));
        if(++br%4==0) fprintf(out, "\n");
      }
      if(br%4!=0) fprintf(out, "\n");
    }
  }
}

int main(int argc, char* argv[]){
  string in;
  string out;
  if (argc == 4 && strcmp(argv[0], "./assembler")==0 && strcmp(argv[1], "-o")==0){
    in = argv[3];
    out = argv[2];
  }
  else{
    cerr << "Greska, pogresni argumenti" << endl;
    return -1;
  }
	FILE* inFile = fopen(in.c_str(), "r");
  if(inFile==NULL){
    cerr << "Greska, pogresan ulazni fajl" << endl;
    return -1;
  }
  FILE* outFile = fopen(out.c_str(), "w");

  if(outFile==NULL){
    cerr << "Greska, pogresan izlazni fajl" << endl;
    return -1;
  }
	yyin = inFile;

	if (yyparse())
		return 1;
	

  pass1();
  middle_pass();
  pass2();
  print_into_file(outFile);
	instr_list.clear();
  arg_list.clear();
	
	return 0;
}
