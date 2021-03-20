import com.fazecast.jSerialComm.SerialPort;

import javax.swing.*;
import javax.swing.filechooser.FileFilter;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Scanner;

public class MainGUI {

    SerialPort port;
    private JPanel mainPanel;
    private JButton connectButton;
    private JComboBox<String> cb_comports;
    private JButton exitButton;
    private JTextField tf_file;
    private JButton chooseFileButton;
    private JTextField tf_cmd;
    private JButton sendCommandButton;
    private JButton startJobButton;
    private JMenuBar menu;
    private JButton clearWindowButton;
    private JTextArea textArea1;
    volatile boolean cmd_executed;

    public MainGUI(){
        JFrame mainWindow = new JFrame();

        JMenu Creator = new JMenu("GCode Creator..");
        JMenuItem openCreator = new JMenuItem("Start GCode Creator");
        Creator.add(openCreator);
        menu.add(Creator);

        SerialPort[] portNames = SerialPort.getCommPorts();
        for (int i = 0; i < portNames.length; i++) {
            cb_comports.addItem(portNames[i].getSystemPortName());}
        if (cb_comports.getItemCount() > 0){
            connectButton.setEnabled(true);
        }

        openCreator.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                new GcodeCreator();
            }
        });

        connectButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                if (connectButton.getText().equals("Connect")) {
                    port = SerialPort.getCommPort(cb_comports.getSelectedItem().toString());

                    port.setComPortTimeouts(SerialPort.TIMEOUT_SCANNER, 0, 0);
                    if(port.openPort()) {
                        port.setBaudRate(115200);
                        addToWindow("Machine Connected","SYSTEM");
                        connectButton.setText("Disconnect");
                        sendCommandButton.setEnabled(true);
                        startJobButton.setEnabled(true);
                        Thread thread = new Thread(() -> {
                            Scanner scanner = new Scanner(port.getInputStream());

                            while (scanner.hasNextLine()) {
                                String line = scanner.nextLine();

                                if (line.equals("OK")) {
                                    cmd_executed = true;
                                }else{
                                    addToWindow(line, "MACHINE");
                                }
                            }
                        });
                        thread.start();
                    }else{
                        addToWindow("Connection failed", "SYSTEM");
                    }
                }else{
                    addToWindow("Machine disconnected", "SYSTEM");
                    writeToPort("M5");
                    port.closePort();
                    sendCommandButton.setEnabled(false);
                    startJobButton.setEnabled(false);
                    connectButton.setText("Connect");
                }
            }
        });
        exitButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                mainWindow.dispose();
                System.exit(0);
            }
        });
        Action sendCmd = new AbstractAction()
        {
            @Override
            public void actionPerformed(ActionEvent e)
            {
                writeToPort(tf_cmd.getText());
            }
        };
        sendCommandButton.addActionListener(sendCmd);
        tf_cmd.addActionListener(sendCmd);
        chooseFileButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                JFileChooser chooser = new JFileChooser(new File("user.dir"));
                chooser.setFileFilter(new FileFilter() {

                    public String getDescription() {
                        return "txt files (*.txt)";
                    }

                    public boolean accept(File f) {
                        if (f.isDirectory()) {
                            return true;
                        } else {
                            String filename = f.getName().toLowerCase();
                            return filename.endsWith(".txt");
                        }
                    }
                });
                int choice = chooser.showSaveDialog(null);
                if(choice == JFileChooser.APPROVE_OPTION) {
                    tf_file.setText(chooser.getSelectedFile().getPath());

                }
            }
        });
        clearWindowButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent e) {
                textArea1.setText("");
            }
        });
        startJobButton.addActionListener(new ActionListener() {
            @Override
            public synchronized void actionPerformed(ActionEvent e) {
                if (startJobButton.getText().equals("Start Job")) {

                    Thread runTask = new Thread(() -> {
                        BufferedReader reader;
                        System.out.println("Starting Task");
                        try {
                            reader = new BufferedReader(new FileReader(tf_file.getText()));
                            String line;
                            startJobButton.setText("Stop Job");
                            while ((line = reader.readLine()) != null && startJobButton.getText().equals("Stop Job")) {
                                writeToPort(line);

                                while (!cmd_executed){}
                                }
                            reader.close();
                            startJobButton.setText("Start Job");
                        } catch (IOException ignored) {}
                    });
                    runTask.start();
                }else{
                    startJobButton.setText("Start Job");
                }
            }
            });
        //end of constructor
        mainWindow.add(mainPanel);
        mainWindow.setPreferredSize(new Dimension(1000,800));
        mainWindow.setTitle("GalvoStep Control Suite");
        mainWindow.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
        mainWindow.pack();
        mainWindow.setVisible(true);
    }

    public static void main(String[] args){
        new MainGUI();
    }

    public void writeToPort(String input){
        input += '\n';
        port.writeBytes(input.getBytes(StandardCharsets.UTF_8), input.length());
        cmd_executed = false;
    }

    public void addToWindow(String line, String from){
        Date time = new Date();
        SimpleDateFormat formatter = new SimpleDateFormat("HH:mm:ss");
        textArea1.append(formatter.format(time)+" <"+from+">: "+ line+ "\n");
    }
}
