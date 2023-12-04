#include "../inc/linker.hpp"


void add_place(char* name){
  string a = name;
  int m=0, mm=0;
  while(a[m++]!='=');
  mm=m;
  while(a[m++]!='@');
  string n = a.substr(mm,m-mm-1);
  place[n] = stol(a.substr(m), 0, 16);
}

void make_symbol_table(FILE* in, int id){
  fseek(in,0,SEEK_SET);
  char * line = NULL;
  size_t len = 0;
  int num; int value; int sz, offset, add, add2;
  char type[6]; char bind[5]; char name[25]; char ndx[20];
  unordered_map<const char*, int> add_sec;
  list<Section*> cur;
  int cnt=0;
  int read = getline(&line, &len, in);
  if(read!=-1){
    if(strcmp(line, "--> TABELA SIMBOLA <--\n")!=0) { cerr<<"gr\n"; exit(-1);} //greska
    else{
      while ((read = getline(&line, &len, in)) != -1 && strcmp(line, "--> RELOKACIJE <--\n")!=0) {
        if(strcmp(line,"\n")!=0){
          char* t = new char[6], *b=new char[5], *n=new char[25], *nd=new char[25];
          sscanf(line, "%d %d %d %s %s %s %s\n", &num, &value, &sz, type, bind, ndx, name);
          Symbol* i = new Symbol(num, value, strcpy(t,&type[0]), strcpy(b, &bind[0]), strcpy(nd, &ndx[0]), strcpy(n,&name[0]));
          i->size=sz;
          if(strcmp(i->type, "SCTN")==0) {
            if(!in_symbol_table(i->name)){
              symbols_table.push_back(i);
              Section* sec = new Section(i->name, 0);
              sec->size=i->size;
              sections.push_back(sec);
            }
            else{
              add_sec[i->ndx]=i->size;
            }
          }
          else if(strcmp(i->ndx,"UND")!=0){
            if(!add_sec.empty()){
              for(Symbol* s:symbols_table){
                if(strcmp(s->name, i->ndx)==0){
                  i->value+=s->size;
                  break;
                }
              }
            }
            if(!in_symbol_table(i->name)){
              symbols_table.push_back(i);
            }
            else{
              cerr<<"Greska, visestruko definisan simbol: "<<i->name<<endl;
              exit(-1);
            }
          }
          else if(strcmp(i->ndx,"UND")==0) undefined_symbols.push_back(i);
        }
      }
      while ((read = getline(&line, &len, in)) != -1 && strcmp(line, "--> SEKCIJE <--\n")!=0 ) {
        if(strcmp(line,"\n")!=0){
          char* t = new char[20], *n=new char[20];
          sscanf(line, "%d %s %s %d %d\n", &offset, type, name, &add, &add2);
          Reloc* r = new Reloc(offset, strcpy(t,&type[0]), strcpy(n, &name[0]), add, add2);
          if(!add_sec.empty()){
            for(Symbol* s:symbols_table){
              if(strcmp(s->name, r->type)==0){
                r->offset+=s->size;
                break;
              }
            }
          }
          relocation_table.push_back(r);
        }
      }

      for(auto a: add_sec){
        for(Symbol* s: symbols_table){
          if(strcmp(s->name, a.first)==0) { 
            s->size+=a.second; 
            for(Section* ss: sections){
              if(strcmp(ss->name, s->name)==0){
                ss->size+=a.second;
                break;
              }
            }
            break;
          }
        }
      }

      while((read = getline(&line, &len, in))!=-1){
        if(strcmp(line,"\n")!=0){
          int sl = strlen(line);
          char* nov = new char[sl];
          strncpy(nov, line, sl-1);
          nov[sl-1]='\0';
          int br=0;
          for(Section* s: sections){
            if(strcmp(s->name,nov)==0){
              while ((read = getline(&line, &len, in)) != -1 && strcmp(line, "\n")!=0) {
                int b0, b1, b2, b3;
                sscanf(line, "%x %x %x %x\n", &b0, &b1, &b2, &b3);
                s->section_mem.push_back((char) (b0 & 0xff)); 
                s->section_mem.push_back((char) (b1 & 0xff)); 
                s->section_mem.push_back((char) (b2 & 0xff)); 
                s->section_mem.push_back((char) (b3 & 0xff));
              }
              while(s->section_mem.size()>s->size) s->section_mem.pop_back();
              break;
            }
          }
        }
      }
    }
  }
}
bool in_symbol_table(const char* name){
  //cout<<name<<endl;
  for(Symbol* s:symbols_table){
    if(strcmp(s->name, name)==0) return true;
  }
  return false;
}
bool is_undefined(const char* name){
  for(auto x: undefined_symbols){
    if(strcmp(x->name, name)==0){
      return true;
    }
  }
  return false;
}

bool compare_sections(const Section* s1, const Section* s2) {
  if(s1->start_adr>0 && s2->start_adr>0) return s1->start_adr<s2->start_adr;
  if(s1->start_adr>0 && s2->start_adr<0) return true;
  if(s1->start_adr<0 && s2->start_adr>0) return false;
  return s1->start_adr<s2->start_adr;
}

void print_hex(FILE* out){
  sections.sort(compare_sections);
  int last=0;
  for (auto it = sections.begin(); it != sections.end(); ++it){
    if((*it)->section_mem.size()>0){
      int st = (*it)->start_adr;
      if(last!=st){
        if(last%4!=0){
          while(last%4!=0){
            fprintf(out, "00 ");
            last++;
          }
          fprintf(out, "\n");
        }
      }
      for(auto const& m: (*it)->section_mem){
        if(st%4==0) fprintf(out, "%.8x: ", st);
        fprintf(out, "%.2x ", (unsigned int)(m & 0xFF));
        if(++st%4==0) fprintf(out, "\n");
      }
      last=(*it)->start_adr+(*it)->section_mem.size();
    }
  }
  if(last%4!=0) fprintf(out, "\n");
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


void add_st_adr(){
  for(auto pl: place){
    for(Section* sec:sections){
      if(pl.first.compare(sec->name)==0){
        sec->start_adr = pl.second;
        if((max_adr>0 && sec->start_adr<max_adr) || (max_adr<0 && sec->start_adr<max_adr)){
          cerr<<"Greska, preklapanje sekcija: "<<sec->name<<endl;
          exit(-1);
        }
        if(sec->start_adr+sec->size>0 && max_adr<sec->start_adr+sec->size && max_adr>=0) max_adr=sec->start_adr+sec->size;
        else if(sec->start_adr<0 && max_adr>sec->start_adr-sec->size) max_adr=sec->start_adr+sec->size;
        break;
      }
    }
  }
  for(Section* sec: sections){
    if(sec->start_adr==0){
      sec->start_adr=max_adr;
      max_adr+=sec->size;
    }
    for(Symbol* s: symbols_table){
      if(strcmp(s->ndx,sec->name)==0) {
        s->value+=sec->start_adr;
      }
    }
  }
}
void resolve_reloc(){
  for(auto it: relocation_table){
    int val = 0;
    for(Symbol* sim: symbols_table){
      if(strcmp(sim->name, it->symbol_name)==0){
        val=sim->value;
      }
    }

    for(Section* sec: sections){
      if(strcmp(sec->name, it->type)==0){
        auto mem = sec->section_mem.begin();
        advance(mem, it->offset+2);
        if(it->addend2==4){
          char b0 = *mem & 0xf; mem++; //20
          char b1 = *mem;
          int new_off = (b0<<4 & 0xf00) | (b1 & 0xff);
          new_off+=it->offset;
          mem = sec->section_mem.begin();
          advance(mem, new_off);
        }
        *mem = ((val)&0xff); ++mem;
        *mem = ((val>>8)&0xff); ++mem;
        *mem = ((val>>16)&0xff); ++mem;
        *mem = ((val>>24)&0xff); ++mem;
        break;
      }
    }
  }
}
void resolve_reloc_r(){
   for(auto it: relocation_table){
    int val = 0; bool abs=false;
    for(Symbol* sim: symbols_table){
      if(strcmp(sim->name, it->symbol_name)==0){
        val=sim->value;
      }
    }
    for(Section* sec: sections){
      if(strcmp(sec->name, it->type)==0){
        auto mem = sec->section_mem.begin();
        advance(mem, it->offset+2);
        if(it->addend2==4){
          char b0 = *mem & 0xf; mem++; //20
          char b1 = *mem;
          int new_off = (b0<<4 & 0xf00) | (b1 & 0xff);
          new_off+=it->offset;
          mem = sec->section_mem.begin();
          advance(mem, new_off);
        }
        *mem = ((val)&0xff); ++mem;
        *mem = ((val>>8)&0xff); ++mem;
        *mem = ((val>>16)&0xff); ++mem;
        *mem = ((val>>24)&0xff); ++mem;
        break;
      }
    }
  }
}
void clear_undefined(){
  auto i = undefined_symbols.begin();
  while(i!=undefined_symbols.end()){
    if(in_symbol_table((*i)->name)){
      undefined_symbols.erase(i++);
    }
    else ++i;
  }
}

int main(int argc, char* argv[]){
  string in;
  string out = "main.hex";
  list<FILE*> in_files;
  
  if(argc>1 && strcmp(argv[0],"./linker")==0){
    for(int i=1; i<argc; ){
      if(strcmp(argv[i],"-hex")==0){ i++; type=HEX;}
      else if(strcmp(argv[i],"-relocatable")==0){ i++; type=RELOC;}
      else if(strcmp(argv[i],"-o")==0){
        out = argv[++i];
        i++;
      }
      else{
        string a = argv[i];
        a.resize(7);
        if(a.compare("-place=")==0){
          add_place(argv[i]);
          i++;
        }
        else{
          in=argv[i];
          FILE* inFile = fopen(in.c_str(), "r");
          if(inFile==NULL){
            cerr << "Greska, pogresan ulazni fajl" << endl;
            return -1;
          }
          in_files.push_back(inFile);
          i++;
        }
      }
    }
  }
  else{
    cerr << "Greska, pogresni argumenti" << endl;
    return -1;
  }
  FILE* outFile = fopen(out.c_str(), "w");

  if(outFile==NULL){
    cerr << "Greska, pogresan izlazni fajl" << endl;
    return -1;
  }
  if(type==HEX){
    int id=1;
    for(FILE* in_file: in_files){
      make_symbol_table(in_file, id++);
    }
    clear_undefined();

    if(undefined_symbols.size()>0){
      cerr<<"Greska, nedefinisan simbol/i: ";
      for(Symbol* simbol: undefined_symbols){
        cerr<<simbol->name<<" ";
      }
      cerr<<endl;
      exit(-1);
    }
    add_st_adr();
    resolve_reloc();
    print_hex(outFile);
  }
  else if(type==RELOC){
    int id=1;
    place.clear();
    for(FILE* in_file: in_files){
      make_symbol_table(in_file, id++);
    }
    //add_st_adr();
    //resolve_reloc_r();
    print_into_file(outFile);
  }
  else{
    cerr<<"Greska, linker mora sadrzati -hex or -relocatable\n";
    exit(-1);
  }

  symbols_table.clear();
  undefined_symbols.clear();
  sections.clear();
  pl.clear();
  relocation_table.clear();
  place.clear();
  file_sections.clear();
}