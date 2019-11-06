// $Id: commands.cpp,v 1.18 2019-10-08 13:55:31-07 - - $

#include "commands.h"
#include "debug.h"
#include <string>
#include <regex>

using namespace std;

command_hash cmd_hash {
   {"cat"   , fn_cat   },
   {"cd"    , fn_cd    },
   {"echo"  , fn_echo  },
   {"exit"  , fn_exit  },
   {"ls"    , fn_ls    },
   {"lsr"   , fn_lsr   },
   {"make"  , fn_make  },
   {"mkdir" , fn_mkdir },
   {"prompt", fn_prompt},
   {"pwd"   , fn_pwd   },
   {"rm"    , fn_rm    },
};

command_fn find_command_fn (const string& cmd) {
   // Note: value_type is pair<const key_type, mapped_type>
   // So: iterator->first is key_type (string)
   // So: iterator->second is mapped_type (command_fn)
   DEBUGF ('c', "[" << cmd << "]");
   const auto result = cmd_hash.find (cmd);
   if (result == cmd_hash.end()) {
      throw command_error (cmd + ": no such function");
   }
   return result->second;
}

command_error::command_error (const string& what):
            runtime_error (what) {
}

int exit_status_message() {
   int status = exec::status();
   cout << exec::execname() << ": exit(" << status << ")" << endl;
   return status;
}

void fn_cat (inode_state& state, const wordvec& words){
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   int i = 0;
   int size = words.size();
   wordvec fpath;
   bool inroot = false;
   if (size < 2) {
     throw command_error("cat: too few operands");
   }
   
   DEBUGF ('+', "checked arg size");
   for (i = 1; i < size; ++i) {
      DEBUGF ('+', "started split");
      fpath = split(words.at(i), "/");
      DEBUGF ('+', "finished split");
      if (state.current_dir()->get_iname() == "") {
         inroot = true;
      }
      DEBUGF ('+', "checking validity");
      inode dest  = *validity(state, fpath, inroot);
      DEBUGF ('+', "valid")
      if (dest.get_ftype() == file_type::PLAIN_TYPE) {
        DEBUGF ('+', "tried to print");
        cout << dest << endl;
      } else {
        throw command_error("cat : this is a directory");
      }        
   }
}

void fn_cd (inode_state& state, const wordvec& words){
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   int size = words.size();
   wordvec fpath;
   inode_ptr dest = nullptr;
   bool inroot = false;

   if (size > 2) {
     throw command_error("ls : too many operands");
   }

   if (size == 2) {
     fpath = split(words.at(1), "/");
     if (state.current_dir()->get_iname() == "") {
       inroot = true;
     }
     dest = validity(state, fpath, inroot);
     state.set_dir(dest);
   } else {
     state.set_dir(state.get_root());
   }

   return;
}

void fn_echo (inode_state& state, const wordvec& words){
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   cout << word_range (words.cbegin() + 1, words.cend()) << endl;
}


void fn_exit (inode_state& state, const wordvec& words){
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   int i = 0;
   int status = 0;
   int size = words.size();
   int asize = 0;
   string arg;
   
   if (size > 1) {
      DEBUGF ('-', "size > 1");
      arg = words.at(1);
      asize = arg.size();

      for (i = 0; i < asize; ++i) {
         if (arg.at(i) < '0' or arg.at(i) > '9') {
            status = 127;
            break;
         }
      }
 
      if (status != 127) {
         status = stoi(arg);
      } 
   }
   
   DEBUGF ('-', "about to start recursive remove");
   DEBUGF ('-', "root");   
   bottom_remove(state.get_root());
   DEBUGF ('-', "finished recursive remove");
   _Exit(status);
   throw ysh_exit();
}

void fn_ls (inode_state& state, const wordvec& words){
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   int size = words.size();
   int i = 0;
   wordvec fpath;
   bool inroot = false;
   if (size > 1) {
      for (i = 0; i < size; ++i) {
         fpath = split(words.at(i), "/");

         if (state.current_dir()->get_iname() == "") {
           inroot = true;
         }
         inode dest = *validity(state, fpath, inroot);

         cout << dest << endl;
      }
   } else {
      inode cdir = *state.current_dir();
      cout << cdir << endl;
   }
   
   return;
}

void bottom_print(inode_ptr iptr) {
   cout<< *iptr << endl;
   wordvec children = iptr->get_children();
   int i = 0;
   int size = children.size();

   for (i = 2; i < size; ++i) {
      inode_ptr kid = iptr->child_dir(children.at(i));
     
      if (kid->get_ftype() == file_type::DIRECTORY_TYPE) {
         bottom_print(kid);
      }
   }

   return;
}

void fn_lsr (inode_state& state, const wordvec& words){
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   int size = words.size();
   int i = 0;
   wordvec fpath;
   bool inroot = false;
   if (size > 1) {
      for (i = 0; i < size; ++i) {
         fpath = split(words.at(i), "/");
         if (state.current_dir()->get_iname() == "") {
            inroot = true;
         }
         inode_ptr dest = validity(state, fpath, inroot);
         
         bottom_print(dest);
      }
   } else {
      inode_ptr cdir = state.current_dir();
      bottom_print(cdir);
   }

   return;
  
}

void fn_make (inode_state& state, const wordvec& words){
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   int size = words.size();
   inode_ptr nfile = nullptr;
   wordvec fcontents;   

   if (size < 2) {
     throw command_error("make : too few operands");
   }

   nfile = file_maker(state, words, false);

   fcontents = words;
   fcontents.erase(fcontents.begin());
   fcontents.erase(fcontents.begin());
   nfile->writefile(fcontents);
}

void fn_mkdir (inode_state& state, const wordvec& words){
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   int size = words.size();


   if (size < 2) {
      throw command_error("mkdir : too few operands");    
   } 

   if (size > 2) {
      throw command_error("mkdir : too many operands");     
   }  
  
   file_maker(state, words, true);    
  
}

void fn_prompt (inode_state& state, const wordvec& words){
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   int i = 0;
   int size = words.size();
   string str = "";
   if (size > 1) {
      for (i = 1; i < size; ++i) {
         str = str + words[i] + " ";
      }

      state.change_prompt(str);
   }
}

void fn_pwd (inode_state& state, const wordvec& words){
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   int i = 0;
   int size = 0;
   inode_ptr here = state.current_dir();
   wordvec cpath;
   inode_ptr rt = state.get_root();
   
   while (here != rt) {
      cpath.insert(cpath.begin(), here->get_iname());
      here = here->get_parent();
   }
   
   size = cpath.size();

   for (i = 0; i < size; ++i) {
      cout << "/" << cpath.at(i);
   } 

   if (size ==  1) {
     cout<< "/";
   }

   cout << endl;
   
   return; 
}

void fn_rm (inode_state& state, const wordvec& words){
   DEBUGF ('c', state);
   DEBUGF ('c', words);
   int size = words.size();
   wordvec fpath;
   wordvec looking;
   bool inroot = false;
   if (size < 2) {
      throw command_error("rm : too few operands");
   }
   
   fpath = split(words.at(1), "/");
   looking = fpath;
   
   if (state.current_dir()->get_iname() == "") {
     inroot = true;
   }
   looking.pop_back();

   inode_ptr dest = validity(state, looking, inroot);

   dest->remove(fpath.back());
 
   return;
}

void bottom_remove(inode_ptr iptr) {
   int i = 0;
   int size = 0;
   inode_ptr kid = nullptr;
   if (iptr->get_ftype() == file_type::DIRECTORY_TYPE) {
      wordvec children = iptr->get_children();
      
      size = children.size();
      DEBUGF ('-', "size : " << size);      
      for (i = 2; i < size; ++i) {
         kid = iptr->child_dir(children.at(i));
         if (kid->get_ftype() != file_type::PLAIN_TYPE) {
            bottom_remove(kid);
         } 
      }
   }

   DEBUGF ('-', "about to delete file" << iptr->get_iname());
   DEBUGF ('-', "parent of file" << iptr->get_parent()->get_iname());
   if (iptr->get_iname() != "") {
       iptr->get_parent()->remove(iptr->get_iname());
   }
   DEBUGF ('-', "no problem yet");
   return;
}

void fn_rmr (inode_state& state, const wordvec& words){
   DEBUGF ('c', state);
   DEBUGF ('c', words);

   int size = words.size();
  
   if (size < 2) {
     throw command_error("rmr : too few operands");
   }

   wordvec fpath = split(words.at(1), "/");

   bool inroot = false; 
 
   if (state.current_dir()->get_iname() == "") {
     inroot = true;
   }
   inode_ptr dest = validity(state, fpath, inroot);

   bottom_remove(dest);

   return;
}

inode_ptr validity(inode_state& state,const wordvec path,
                                                 bool is_root) {
      inode_ptr pos;
      int depth = 0;
      int size = path.size();
      
      DEBUGF ('+', "size()" << size);
      if (is_root) {
         DEBUGF ('c', "is_root");
         pos = state.get_root();
      } else {
         DEBUGF ('+', "!is_root");
         pos = state.current_dir();
         DEBUGF ('+', "cwd name: " << pos->get_iname());
      }

      while (depth < size) {
         DEBUGF ('+', "deep in while ");
         try {
           DEBUGF ('+', "try " << path.at(depth));
           pos = pos->child_dir(path.at(depth));
           ++depth;
         } catch (...) {
           DEBUGF ('+', "catch ");
           throw file_error("file system : path does not exist");
         }
        
      }
      DEBUGF ('c', "about to return pos");
      return pos;      
}

inode_ptr file_maker(inode_state& state,const wordvec& words,
                     bool dir) {
      DEBUGF ('c', "in file_maker()");
      wordvec fpath = split(words[1], "/");
      wordvec holder = fpath;
      bool inroot = false;
      //node_ptr dest = nullptr;
      inode_ptr nfile = nullptr;
      DEBUGF ('c', "in file_maker() : inroot" << inroot);     
      holder.erase(holder.end());
            

      if (state.current_dir()->get_iname() == "") {
        inroot = true;
        DEBUGF ('c', "inroo made true " << inroot);
        DEBUGF ('c', state.current_dir()->get_iname());
      }
      inode_ptr dest = validity(state, holder, inroot);
      DEBUGF ('c', "in file_maker() : returned");
      if (dir) {
        return dest->make_dir(fpath.back());
      } else {
        return dest->make_file(fpath.back());
      }

}

