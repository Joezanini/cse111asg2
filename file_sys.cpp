// $Id: file_sys.cpp,v 1.7 2019-07-09 14:05:44-07 - - $

#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <regex>

using namespace std;

#include "debug.h"
#include "file_sys.h"

int inode::next_inode_nr {1};

struct file_type_hash {
   size_t operator() (file_type type) const {
      return static_cast<size_t> (type);
   }
};

ostream& operator<< (ostream& out, file_type type) {
   static unordered_map<file_type,string,file_type_hash> hash {
      {file_type::PLAIN_TYPE, "PLAIN_TYPE"},
      {file_type::DIRECTORY_TYPE, "DIRECTORY_TYPE"},
   };
   return out << hash[type];
}

inode_state::inode_state() {
   inode rfile(file_type::DIRECTORY_TYPE, "");
   root = make_shared<inode>(rfile);
   cwd = root;

   rfile.set_root(root);
   rfile.set_parent(root);
   DEBUGF ('i', "root = " << root << ", cwd = " << cwd
          << ", prompt = \"" << prompt() << "\"");
}

void inode::set_root(inode_ptr new_root) {
   if (ftype == file_type::PLAIN_TYPE) {
      throw file_error ("is a plain file");
   }

   dynamic_pointer_cast<directory>(contents) -> set_dir(string("."),
                                                       new_root);
}

void inode_state::change_prompt(const string& str) {
   prompt_ = str + " ";
}


const string& inode_state::prompt() const { return prompt_; }

inode_ptr inode::make_dir(string name) {
    DEBUGF ('c', "dest->make_dir");
    
    inode_ptr dir = dynamic_pointer_cast<directory>(contents)->
                                                    mkdir(name);
    DEBUGF ('c', "after dynamic_p_c");
    shared_ptr p = make_shared<inode>(*this);
    DEBUGF ('c', "about to send p");
    dir->set_parent(p);
    DEBUGF ('c', "afer set p");
    return dir;
}

void inode::set_parent(inode_ptr nparent) {
    if (ftype == file_type::PLAIN_TYPE) {
       throw file_error("is a plain file");
    }
    DEBUGF ('c', "setting parent");

    dynamic_pointer_cast<directory>(contents)->set_dir(string(".."),
                                                             nparent);
}

inode_ptr inode::make_file(string name) {
   return dynamic_pointer_cast<directory>(contents)->mkfile(name);
}

void inode::writefile(const wordvec& file) {
   if (ftype == file_type::DIRECTORY_TYPE) {
      throw file_error("cannot write file data to a directory");
   }

   dynamic_pointer_cast<plain_file>(contents)->writefile(file);
}

inode_ptr inode_state::get_root() { return root; }

inode_ptr inode_state::current_dir() { return cwd; }

ostream& operator<< (ostream& out, const inode_state& state) {
   out << "inode_state: root = " << state.root
       << ", cwd = " << state.cwd;
   return out;
}

inode::inode(file_type type, string node_nm) :
                                  inode_nr(next_inode_nr++) {
   switch (type) {
      case file_type::PLAIN_TYPE:
           contents = make_shared<plain_file>();
           ftype = type;
           iname = node_nm;
           break;
      case file_type::DIRECTORY_TYPE:
           contents = make_shared<directory>();
           ftype = type;
           iname = node_nm;
           break;
   }
   DEBUGF ('i', "inode " << inode_nr << ", type = " << type);
}

file_type inode::get_ftype() { return ftype; }

ostream& operator<<(ostream& out, inode& node) {
   if (node.ftype == file_type::DIRECTORY_TYPE) {
      out << "/" << node.iname << " : " << endl;
      out << *dynamic_pointer_cast<directory>(node.contents);
   } else {
      out << *dynamic_pointer_cast<plain_file>(node.contents);
   }
   
   return out;
}

int inode::get_inode_nr() const {
   DEBUGF ('i', "inode = " << inode_nr);
   return inode_nr;
}

inode_ptr inode::child_dir(string name) {
   return dynamic_pointer_cast<directory>(contents)->get_dirent(name);
}


file_error::file_error (const string& what):
            runtime_error (what) {
}

const wordvec& base_file::readfile() const {
   throw file_error ("is a " + error_file_type());
}

void base_file::writefile (const wordvec&) {
   throw file_error ("is a " + error_file_type());
}

void base_file::remove (const string&) {
   throw file_error ("is a " + error_file_type());
}

ostream& operator<< (ostream& out, const base_file&) {
    out << "This is a base file type";
    return out;
}

ostream& operator<< (ostream& out, const plain_file& pf) {
    out << pf.data;
    return out;
}

int get_digit_width(int n) {
    int width = 1;
    
    if (n >= 0) {
       while(n > 10) {
         n /= 10;
         ++width;
       }
    }
    
    return width;
}

int inode::size() {
   return contents->size();
}

void inode_state::set_dir(inode_ptr ptr) {
     cwd = ptr;
     return;
}

wordvec inode::get_children() {
    return dynamic_pointer_cast<directory>(contents)->
                                      get_content_labels();
}

string inode::get_iname() {
    return iname;
}

void inode::remove(string fname) {
    contents->remove(fname);
}

inode_ptr inode::get_parent() {
   if (ftype == file_type::PLAIN_TYPE) {
      throw file_error ("is a plain file");
   }
   return dynamic_pointer_cast<directory>(contents)->get_dirent("..");
}

ostream& operator<< (ostream& out, const directory& dir) {
    int inode_num = 0;
    int column1w = 0;
    int column2w = 0;
    int size = 0;
    for (map<string,inode_ptr>::const_iterator it = dir.dirents.begin();
                  it != dir.dirents.end(); ++it) {
       inode_num = it->second->get_inode_nr();
       column1w = 5 - get_digit_width(inode_num);

       while(column1w > 0) {
          out <<" ";
          --column1w;
       } 
      
       out << inode_num;
       out << " ";

       size = it->second->size();
       column2w = get_digit_width(size);

       while(column2w > 0) {
          out << " ";
          --column2w;
       }   
       out << size;
       out << " ";

       out << it->first;
       
       if (it->second->get_ftype() == file_type::DIRECTORY_TYPE
               and !(it->first == "." or it->first == "..")) {
          out << "/";
       }

       if (it != dir.dirents.end()) {
           out << endl;   
       }
    }

    return out;
} 

inode_ptr base_file::mkdir (const string&) {
   throw file_error ("is a " + error_file_type());
}

inode_ptr base_file::mkfile (const string&) {
   throw file_error ("is a " + error_file_type());
}


size_t plain_file::size() const {
   size_t size {data.size()};
   DEBUGF ('i', "size = " << size);
   return size;
}

const wordvec& plain_file::readfile() const {
   DEBUGF ('i', data);
   return data;
}

void plain_file::writefile (const wordvec& words) {
   DEBUGF ('i', words);
   data = words;
}

void plain_file::remove(const string&) {
     throw file_error("this is a file!");
}

inode_ptr plain_file::mkdir(const string&) {
     throw file_error("this is a file!");
}

inode_ptr plain_file::mkfile(const string&) {
     throw file_error("this is a file!");
}

directory::directory() {
   dirents.insert(pair<string, inode_ptr>(".", nullptr));
   dirents.insert(pair<string, inode_ptr>(".", nullptr));
}

directory::directory(inode_ptr root, inode_ptr parent) {
   dirents.insert(pair<string, inode_ptr>(".", root));
   dirents.insert(pair<string, inode_ptr>(".", parent));
}

void directory::writefile(const wordvec&) {
     throw file_error("this is a directory!");
}

const wordvec& directory::readfile() const {
     throw file_error("this is a directory!");
}

size_t directory::size() const {
   size_t size = dirents.size();
   DEBUGF ('i', "size = " << size);
   return size;
}

void directory::remove (const string& filename) {
   DEBUGF ('i', filename);
  
   map<string, inode_ptr>::iterator it = dirents.find(filename);

   if (it != dirents.end()) {
      inode_ptr rnode = dirents.at(filename);

      if (rnode->get_ftype() == file_type::DIRECTORY_TYPE) {
         if (rnode->size() > 2) {
            throw file_error(filename +
                       " cannot be removed because it is not empty");
         }

         rnode->set_root(nullptr);
         rnode->set_parent(nullptr);
      }

      dirents.erase(it);
   } else {
      throw file_error (filename + 
                 " cannot be removed because it does not exist");
   }
   return;
}

inode_ptr directory::mkdir (const string& dirname) {
   DEBUGF ('i', dirname);
   map<string, inode_ptr>::iterator it = dirents.find(dirname);

   if (it != dirents.end()) {
      throw file_error(dirname + " already exists");
   }

   inode ndir(file_type::DIRECTORY_TYPE, dirname);

   ndir.set_root(dirents.at("."));

   inode_ptr dptr = make_shared<inode>(ndir);

   dirents.insert(pair<string, inode_ptr>(dirname, dptr));

   return dptr;
}

inode_ptr directory::mkfile (const string& filename) {
   DEBUGF ('i', filename);
   
   map<string, inode_ptr>::iterator it = dirents.find(filename);

   if (it != dirents.end()) {
      return dirents.at(filename);
   }

   inode nfile(file_type::PLAIN_TYPE, filename);

   inode_ptr fptr = make_shared<inode>(nfile);

   dirents.insert(pair<string, inode_ptr>(filename, fptr));

   return fptr;
}

inode_ptr directory::get_dirent(string name) {
   return dirents.at(name);
}

void directory::set_dir(string dname, inode_ptr dir) {
   DEBUGF ('c', "set_dir mapped");
   map<string, inode_ptr>::iterator itr = dirents.find(dname);
   if (itr != dirents.end()) {
      dirents.erase(dname);
   }

   dirents.insert(pair<string, inode_ptr>(dname, dir));
   return;
}

wordvec directory::get_content_labels() {
   wordvec labels;
   
   for (map<string, inode_ptr>::iterator it = dirents.begin();
                  it != dirents.end(); ++it) {
       labels.push_back(it->first);
   }
   
   return labels;
}

