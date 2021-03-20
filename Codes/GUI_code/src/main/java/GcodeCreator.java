import javax.imageio.ImageIO;
import javax.swing.*;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.filechooser.FileFilter;
import javax.swing.filechooser.FileNameExtensionFilter;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.image.BufferedImage;
import java.awt.image.WritableRaster;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.text.DecimalFormat;
import java.util.Locale;

public class GcodeCreator extends JDialog {
    private JPanel mainPanel;
    private JButton createGCodeButton;
    private JButton goBackButton;
    private JPanel picturePane;
    private JPanel controlPane;
    private JTextField im_path;
    private JButton chooseFileButton;
    private JSpinner xlength;
    private JSpinner feedrate;
    private JSpinner jograte;
    private JSpinner spotsize;
    private JLabel picLabel;
    private JTextField outputdir;
    private JButton chooseDirectoryButton;
    private JTextField fileName;
    private static final DecimalFormat df = new DecimalFormat("0.00");
    public int xbound = 500;
    public int ybound = 500;
    public ImageIcon curr_img;
    public String fileDir;

    public GcodeCreator(){
        xlength.setValue(35);
        feedrate.setValue(95);
        jograte.setValue(400);
        spotsize.setValue(160);

        goBackButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                GcodeCreator.super.dispose();
            }
        });

        chooseFileButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                JFileChooser chooser = new JFileChooser(new File("user.dir"));
                chooser.setAcceptAllFileFilterUsed(false);
                FileNameExtensionFilter filter = new FileNameExtensionFilter("Pictures", "png", "jpg", "jpeg");
                chooser.addChoosableFileFilter(filter);
                int choice = chooser.showSaveDialog(null);
                if(choice == JFileChooser.APPROVE_OPTION) {
                    im_path.setText(chooser.getSelectedFile().getPath());
                }
            }
        });
        im_path.getDocument().addDocumentListener(new DocumentListener() {
            public void changedUpdate(DocumentEvent e) {
                updatePicture();
            }
            public void removeUpdate(DocumentEvent e) {
                updatePicture();
            }
            public void insertUpdate(DocumentEvent e) {
                updatePicture();
            }

            public void updatePicture() {
                String path = im_path.getText();
                File pic = new File(path);
                if (pic.isFile()){

                    try {

                        BufferedImage image = ImageIO.read(pic);
                        BufferedImage result = new BufferedImage(
                                image.getWidth(),
                                image.getHeight(),
                                BufferedImage.TYPE_BYTE_BINARY);
                        Graphics2D graphic = result.createGraphics();
                        graphic.drawImage(image, 0, 0, Color.WHITE, null);
                        graphic.dispose();
                        ImageIcon icon = new ImageIcon(result);
                        Image img = icon.getImage();
                        float factor;
                        if (image.getHeight()<image.getWidth()){ //landscape
                            factor = (float) xbound/image.getWidth();
                        }else{
                            factor = (float) ybound/image.getHeight();
                        }
                        int new_x = Math.round(factor*image.getWidth());
                        int new_y = Math.round(factor*image.getHeight());
                        Image newimg = img.getScaledInstance(new_x, new_y,  java.awt.Image.SCALE_SMOOTH);
                        icon = new ImageIcon(newimg);
                        picLabel.setIcon(icon);
                        curr_img = icon;
                        createGCodeButton.setEnabled(true);
                    } catch (IOException e) {
                        e.printStackTrace();
                    }

                }
            }
        });
        createGCodeButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                int feed = readFeed();
                int jog = readJog();
                int spotsize = readSpotsize();
                int xlength = readxlength();
                float ylength = (((float)curr_img.getIconHeight()/(float) curr_img.getIconWidth())*xlength);
                if (feed == -1 || jog ==-1 || spotsize ==-1){
                    return;
                }
                if (fileName.getText().equals("")){
                    JOptionPane.showMessageDialog(null,"No project name was given!","Error",
                            JOptionPane.ERROR_MESSAGE);
                    return;
                }
                String fullpath;
                if (!outputdir.getText().equals("")){
                    fullpath = outputdir.getText()+"//"+fileName.getText()+".txt";
                }else{
                    fullpath = fileName.getText()+".txt";
                }
                BufferedImage img = new BufferedImage(
                        curr_img.getIconWidth(),
                        curr_img.getIconHeight(),
                        BufferedImage.TYPE_BYTE_BINARY);
                Graphics g = img.createGraphics();
                curr_img.paintIcon(null, g, 0, 0);
                g.dispose();
                WritableRaster raster = img.getRaster();
                File gcode = new File(fullpath);
                try {
                    gcode.createNewFile();
                } catch (IOException ioException) {
                    JOptionPane.showMessageDialog(null,"Fatal error whilst creating file!","Error",
                            JOptionPane.ERROR_MESSAGE);
                    return;
                }
                FileWriter myWriter = null;
                boolean laser_on;
                int prev_val;
                float px_per_mm = img.getHeight()/ylength;
                int scan_increment_px = (int) Math.ceil(px_per_mm*(spotsize/1000.0));
                try {
                    myWriter = new FileWriter(fullpath);
                    myWriter.write("G28\n");
                    myWriter.write("G1 X0 Y0 F");
                    myWriter.write(readFeed()+"\n");
                    myWriter.write("G0 X0 Y0 F");
                    myWriter.write(readJog()+"\n");
                    int width = img.getWidth();
                    int height = img.getHeight();
                    laser_on = false;
                    //Scanning process: j = Row = Y, i = Column = X, prevVal0=black, prevVal255=white
                    //getBLue = 255 = White, getBlue = 0 = Black, since image is binary!
                    for (int j = 0; j < height; j=j+scan_increment_px) {

                        prev_val = 255;
                        if (laser_on) { //Turn laser off if it was on in the previous row
                            myWriter.write("M5\n");
                            laser_on = false;
                        }
                        //Find out if this row contains values:
                        for (int k = 0; k < width; k++){
                            System.out.println(k);
                            if (getBlue(img.getRGB(k,j)) == 0){
                                myWriter.write("G0 X0 Y-" + calc(j, height, ylength)+"\n"); //Move to the column
                                break;
                            }
                        }

                        for (int i = 0; i < width; i++) { //Scanning in Row:
                            if (prev_val == 255 && getBlue(img.getRGB(i, j)) == 0) {  //If pixel turns from white to black:
                                if (i == width - 1 || getBlue(img.getRGB(i+ 1, j )) == 255) {
                                    //if the last single pixel happens to be black, just pulse the laser. Same if the next pixel is white again.
                                    myWriter.write("G0 X" + calc(i, width, xlength) + " Y-" + calc(j, height, ylength) + "\n");
                                    myWriter.write("M6\n");
                                } else { //else (usual case) move in G0 to the position and turn on laser
                                    myWriter.write("G0 X" + calc(i, width, xlength) + " Y-" + calc(j, height, ylength) + "\n");
                                    myWriter.write("M3\n");
                                    laser_on = true;

                                }
                                prev_val = 0;
                            } else if (prev_val == 0 && getBlue(img.getRGB(i, j)) == 255) {
                                //if pixel turn from black to white, move the laser to this position.
                                myWriter.write("G1 X" + calc(i - 1, width, xlength) + " Y-" + calc(j, height, ylength) + "\n");
                                if (laser_on) { //Turn the laser off if it was on.
                                    myWriter.write("M5\n");
                                    laser_on = false;
                                    prev_val = 255;
                                }
                            }
                            else if (getBlue(img.getRGB(i,j))==255){
                                prev_val = 255;
                            }
                        }
                    }
                    myWriter.close();
                } catch (IOException ioException) {
                    ioException.printStackTrace();
                }
                JOptionPane.showMessageDialog(null,"Export done!","Compiler has finished",
                        JOptionPane.INFORMATION_MESSAGE);
            }
        });
        chooseDirectoryButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                JFileChooser chooser = new JFileChooser(new File("user.dir"));
                chooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
                int choice = chooser.showSaveDialog(null);
                if(choice == JFileChooser.APPROVE_OPTION) {
                    String path = chooser.getSelectedFile().getPath();
                    outputdir.setText(path);
                }
            }
        });
        this.setModal(true);
        this.add(mainPanel);
        this.setTitle("GalvoStep GCode-Creator");
        this.setPreferredSize(new Dimension(900, 600));
        this.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
        this.pack();
        this.setVisible(true);



    }
    public int readJog(){
        try{
            int jogSpeed = (int) jograte.getValue();
            return jogSpeed;
        }catch(NumberFormatException nfe){
            JOptionPane.showMessageDialog(null,"jogSpeed contains invalid characters","Error",
                    JOptionPane.ERROR_MESSAGE);
            return -1;
        }
    }
    public int readFeed(){
        try{
            int feed = (int) feedrate.getValue();
            return feed;
        }catch(NumberFormatException nfe){
            JOptionPane.showMessageDialog(null,"Feedrate contains invalid characters","Error",
                    JOptionPane.ERROR_MESSAGE);
            return -1;
        }
    }

    public int readSpotsize(){
        try{
            int lasersize = (int) spotsize.getValue();
            return lasersize;
        }catch(NumberFormatException nfe){
            JOptionPane.showMessageDialog(null,"Spotsize contains invalid characters","Error",
                    JOptionPane.ERROR_MESSAGE);
            return -1;
        }
    }
    public int readxlength(){
        try{
            int xmax = (int) xlength.getValue();
            return xmax;
        }catch(NumberFormatException nfe){
            JOptionPane.showMessageDialog(null,"X Length contains invalid characters","Error",
                    JOptionPane.ERROR_MESSAGE);
            return -1;
        }
    }
    public String calc(int pixel, int dimension, float lengthmm){
        float val =  (((float) pixel/(float)dimension)*lengthmm);
        return String.format(Locale.ROOT, "%.2f", val);
    }
    public int getBlue(int color){
        int blue = color & 0xff;
        return blue;
    }
}
