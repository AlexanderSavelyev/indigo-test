package com.ggasoftware.indigo.legio;

import com.ggasoftware.indigo.*;
import java.awt.Color;
import java.awt.Graphics;
import java.awt.Image;
import java.awt.image.BufferedImage;
import java.awt.image.ImageObserver;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import javax.imageio.ImageIO;
import javax.imageio.stream.MemoryCacheImageInputStream;
import javax.swing.JOptionPane;
import javax.swing.JPanel;


public class MolViewPanel extends JPanel
{
   private BufferedImage image;
   private ImageIO image_io;
   private Indigo indigo;
   private IndigoRenderer indigo_renderer;
   private IndigoObject mol;

   int image_w;
   int image_h;
   
   MolViewPanel( Indigo cur_indigo, IndigoRenderer cur_indigo_renderer )
   {
      //indigo = new Indigo();
      indigo = cur_indigo;
      indigo_renderer = cur_indigo_renderer;
      indigo.setOption("render-output-format", "png");
      indigo.setOption("render-comment-font-size", "14");
      indigo.setOption("render-background-color", "1,1,1");
      indigo.setOption("render-coloring", "1");
      //indigo.setOption("render-query", "1");
      setBackground(new java.awt.Color(255, 255, 255));
   }

   public void setMol( String mol_path )
   {
      try
      {
         if (mol_path.endsWith(".rxn"))
            mol = indigo.loadQueryReactionFromFile(mol_path);
         else
            mol = indigo.loadQueryMoleculeFromFile(mol_path);

         String size_str = "" + image_w + ',' + image_h;
         indigo.setOption("render-image-size", size_str);
         indigo.setOption("render-comment", mol.name());

         byte[] bytes = indigo_renderer.renderToBuffer(mol);
         
         ByteArrayInputStream bytes_is;
         bytes_is = new ByteArrayInputStream(bytes, 0, bytes.length);

         image = image_io.read(new MemoryCacheImageInputStream(bytes_is));

      } catch (Exception ex)
      {
         JOptionPane msg_box = new JOptionPane();
         msg_box.showMessageDialog(this, ex.getMessage(), "Error", JOptionPane.ERROR_MESSAGE);
      }
   }

   public void renderImage()
   {
      try
      {
         String size_str = "" + image_w + ',' + image_h;
         indigo.setOption("render-image-size", size_str);
         byte[] bytes;

         try
         {
            bytes = indigo_renderer.renderToBuffer(mol);
         }
         catch ( Exception ex )
         {
            image = null;
            return;
         }

         ByteArrayInputStream bytes_is;
         bytes_is = new ByteArrayInputStream(bytes, 0, bytes.length);

         image = image_io.read(new MemoryCacheImageInputStream(bytes_is));
      } catch (Exception ex)
      {
         JOptionPane msg_box = new JOptionPane();
         msg_box.showMessageDialog(this, ex.getMessage(), "Error", JOptionPane.ERROR_MESSAGE);
      }
   }

   public void setMol( IndigoObject mol_object )
   {
      if (mol_object == null)
      {
         image = null;
         return;
      }

      mol = mol_object.clone();

      renderImage();
   }

   public void update()
   {
      revalidate();
      repaint();
   }

   public void setImageSize( int width, int height )
   {
      image_w = width;
      image_h = height;
   }

   public class ImageObs implements ImageObserver
   {
      @SuppressWarnings("static-access")
      public boolean imageUpdate(Image img, int infoflags, int x, int y, int width, int height) {
         return true;
      }
   }

   public void paintComponent(Graphics g)
   {
      g.setColor(Color.white);
      g.fillRect(0, 0, getWidth(), getHeight());

      if (image == null)
         return;

      if ((image_w != getWidth()) || (image_h != getHeight()))
      {
         setImageSize(getWidth(), getHeight());
         renderImage();
      }

      double new_im_h = image.getHeight();
      double new_im_w = image.getWidth();

      g.drawImage(image, getWidth() / 2 - (int)new_im_w / 2,
                         getHeight() / 2 - (int)new_im_h / 2,
                         (int)new_im_w, (int)new_im_h, new ImageObs());
   }
}
