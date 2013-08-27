#include "bingo_properties.h"

#include "base_cpp/exception.h"
#include "base_cpp/profiling.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <limits.h>

using namespace bingo;
using namespace indigo;

Properties::Properties ()
{
}

void Properties::create (const char *filename)
{
   _filename = filename;
   std::ofstream property_file(filename);
   _props.clear();
}

void Properties::load (const char *filename)
{
   _filename = filename;
   
   std::string line;
   std::ifstream property_file(filename);
   
   if (property_file.is_open())
   {
      while (property_file.good())
      {
         std::getline(property_file, line);

         if (line.size() == 0)
            continue;
         std::string prop_name, prop_value;
         _parseProperty(line, prop_name, prop_value);
         _props.insert(_PropertyPair(prop_name, prop_value));
      }

      property_file.close();
   }
   else
      throw Exception("Property file missed");
}

void Properties::add (const char *prop_name, const char *value)
{
   profTimerStart(t, "prop_add");
   if (_filename.empty())
      throw Exception("Property file's name wasn't initialized");

   if (_props.find(prop_name) == _props.end())
      _props.insert(_PropertyPair(prop_name, value));
   else
      _props[prop_name].assign(value);

   _rewritePropFile();
}

void Properties::add (const char *prop_name, unsigned long value)
{
   std::ostringstream osstr;
   osstr << value;

   add(prop_name, osstr.str().c_str());
}

const char * Properties::get (const char *prop_name)
{
   if (_props.find(prop_name) == _props.end())
      return 0;

   return _props[prop_name].c_str();
}

unsigned long Properties::getULong (const char *prop_name)
{
   if (_props.find(prop_name) == _props.end())
     throw Exception("Unknown property field");

   unsigned long u_dec;
   std::istringstream isstr(_props[prop_name]);
   isstr >> u_dec;

   return u_dec;
}

unsigned long Properties::getULongNoThrow (const char *prop_name)
{
   try
   {
      return getULong(prop_name);
   }
   catch (Exception &ex)
   {
      return ULONG_MAX;
   }
}

void Properties::_rewritePropFile ()
{
   profTimerStart(t, "rewrite_prop");
   std::map<const std::string, std::string>::iterator it;
   
   std::ofstream property_file;
   property_file.open(_filename.c_str(), std::ios::out);

   profTimerStart(t2, "rewrite_prop_writing");
   for (it = _props.begin(); it != _props.end(); it++)
   {
      property_file << it->first << '=' << it->second << std::endl;
   }

   property_file.close();
}

void Properties::_parseProperty (const std::string &line, std::string &prop_out, std::string &value_out)
{
   int sep = (int)line.find_first_of('=');

   prop_out.assign(line.substr(0, sep));
   value_out.assign(line.substr(sep + 1, std::string::npos));
}
