/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package com.ggasoftware.indigo.chemdiff;

import java.io.File;
import java.util.ArrayList;
import javax.swing.filechooser.FileFilter;

public class MolFileFilter extends FileFilter
{
   ArrayList<String> extensions;
   
   public MolFileFilter()
   {
      extensions = new ArrayList<String>();
   }

   public void addExtension( String ext )
   {
      extensions.add(ext);
   }

   public boolean accept( File file )
   {
      if (file.isDirectory())
         return true;
      String name = file.getName().toLowerCase();

      for (int i = 0; i < extensions.size(); i++)
         if (name.endsWith(extensions.get(i)))
            return true;

      return false;
   }

   public String getDescription()
   {
      String descr_string = new String();
      descr_string = "";

      for (int i = 0; i < extensions.size(); i++)
      {
         descr_string += extensions.get(i);
         if (i != extensions.size() - 1)
            descr_string += ", ";
      }

      return descr_string + " files";
   }
}
