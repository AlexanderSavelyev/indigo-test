/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * OutputTable.java
 *
 * Created on Mar 5, 2011, 10:02:17 PM
 */

package com.ggasoftware.indigo.chemdiff;

import com.ggasoftware.indigo.Indigo;
import com.ggasoftware.indigo.IndigoRenderer;
import com.ggasoftware.indigo.controls.BeanBase;
import com.ggasoftware.indigo.controls.MolClicker;
import com.ggasoftware.indigo.controls.MolRenderer;
import com.ggasoftware.indigo.controls.MolSaver;
import com.ggasoftware.indigo.controls.MultiLineCellRenderer;
import com.ggasoftware.indigo.controls.RenderableObject;
import java.util.ArrayList;
import java.io.File;
import javax.swing.SwingConstants;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableColumn;

/**
 *
 * @author achurinov
 */
public class OutputTable extends BeanBase implements java.io.Serializable, MolTable {
   
   public ArrayList<RenderableMolData> mol_datas = new ArrayList<RenderableMolData>();
   private MolSaver _mol_saver;

   public OutputTable() {
      initComponents();
   }

    /** Creates new form OutputTable */
   public void init(Indigo indigo, IndigoRenderer indigo_renderer,
               int cell_w, int cell_h, boolean is_reactions)
   {
      initComponents();

      _mol_saver = new MolSaver(indigo);
      _mol_saver.addExtension("cml");
      _mol_saver.addExtension("smi");
      _mol_saver.addExtension("sdf", "sd");

      int idx_column_count = 1; 
      if (_name.compareTo("Coincident Molecules") == 0)
      {
         idx_column_count = 2;
         j_table.setModel(new MolTableModel(2));
      }

      setBorder(javax.swing.BorderFactory.createTitledBorder(_name));

      MolTableModel model = (MolTableModel)j_table.getModel();

      for (int i = 0; i < idx_column_count; i++)
         j_table.getColumn(model.getIdColumnName(i)).setCellRenderer(new MultiLineCellRenderer(SwingConstants.CENTER,
                                                          SwingConstants.CENTER));
      j_table.getColumn("Molecules").setCellRenderer(new MolRenderer(indigo, indigo_renderer,
              cell_w, cell_h, is_reactions));

      setBorder(javax.swing.BorderFactory.createTitledBorder(_name));

      j_table.getColumn("Molecules").setPreferredWidth(cell_w);
      for (int i = 0; i < idx_column_count; i++)
         j_table.getColumn(model.getIdColumnName(i)).setPreferredWidth(30);
      j_table.addMouseListener(new MolClicker(indigo, indigo_renderer));
   }

    /** This method is called from within the constructor to
     * initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is
     * always regenerated by the Form Editor.
     */
    @SuppressWarnings("unchecked")
   // <editor-fold defaultstate="collapsed" desc="Generated Code">//GEN-BEGIN:initComponents
   private void initComponents() {

      save_panel = new javax.swing.JPanel();
      save_button = new javax.swing.JButton();
      scroll_panel = new javax.swing.JScrollPane();
      j_table = new javax.swing.JTable();

      setBorder(javax.swing.BorderFactory.createTitledBorder("Concident Molecules"));

      save_button.setText("Save");
      save_button.setMaximumSize(new java.awt.Dimension(120, 26));
      save_button.setMinimumSize(new java.awt.Dimension(120, 26));
      save_button.setPreferredSize(new java.awt.Dimension(120, 26));
      save_button.addActionListener(new java.awt.event.ActionListener() {
         public void actionPerformed(java.awt.event.ActionEvent evt) {
            save_buttonActionPerformed(evt);
         }
      });
      save_panel.add(save_button);

      j_table.setModel(new MolTableModel(1));
      j_table.setFocusTraversalPolicyProvider(true);
      j_table.setRowHeight(160);
      scroll_panel.setViewportView(j_table);

      javax.swing.GroupLayout layout = new javax.swing.GroupLayout(this);
      this.setLayout(layout);
      layout.setHorizontalGroup(
         layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
         .addComponent(save_panel, javax.swing.GroupLayout.Alignment.TRAILING, javax.swing.GroupLayout.DEFAULT_SIZE, 529, Short.MAX_VALUE)
         .addComponent(scroll_panel, javax.swing.GroupLayout.DEFAULT_SIZE, 529, Short.MAX_VALUE)
      );
      layout.setVerticalGroup(
         layout.createParallelGroup(javax.swing.GroupLayout.Alignment.LEADING)
         .addGroup(javax.swing.GroupLayout.Alignment.TRAILING, layout.createSequentialGroup()
            .addComponent(scroll_panel, javax.swing.GroupLayout.DEFAULT_SIZE, 388, Short.MAX_VALUE)
            .addPreferredGap(javax.swing.LayoutStyle.ComponentPlacement.RELATED)
            .addComponent(save_panel, javax.swing.GroupLayout.PREFERRED_SIZE, javax.swing.GroupLayout.DEFAULT_SIZE, javax.swing.GroupLayout.PREFERRED_SIZE))
      );
   }// </editor-fold>//GEN-END:initComponents

    private void save_buttonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_save_buttonActionPerformed
       _mol_saver.saveMols(mol_datas);
}//GEN-LAST:event_save_buttonActionPerformed

   public void clear() {
      this.mol_datas.clear();
      
      setBorder(javax.swing.BorderFactory.createTitledBorder(_name));

      DefaultTableModel model = (DefaultTableModel)j_table.getModel();
      while (model.getRowCount() != 0)
         model.removeRow(0);
   }

   public void setMols(ArrayList<RenderableMolData> mol_datas,
              ArrayList< ArrayList<Integer> > indexes1,
              ArrayList< ArrayList<Integer> > indexes2) {
      this.mol_datas.clear();
      this.mol_datas.addAll(mol_datas);

      setBorder(javax.swing.BorderFactory.createTitledBorder(_name + " - " +
                                              mol_datas.size()));

      MolTableModel model = (MolTableModel)j_table.getModel();
      model.setMols(mol_datas, indexes1, indexes2);
   }

   // Variables declaration - do not modify//GEN-BEGIN:variables
   private javax.swing.JTable j_table;
   private javax.swing.JButton save_button;
   private javax.swing.JPanel save_panel;
   private javax.swing.JScrollPane scroll_panel;
   // End of variables declaration//GEN-END:variables
}
