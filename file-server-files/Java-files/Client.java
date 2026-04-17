import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.net.Socket;
import java.nio.charset.StandardCharsets;

public class Client {
    private final int BUFFER_SIZE = 4096;
    private Socket connection;
    private DataInputStream socketIn;
    private DataOutputStream socketOut;
    private int bytes;
    private byte[] buffer = new byte[BUFFER_SIZE];

    public Client(String host, int port, String filename) {
        try {
            connection = new Socket(host, port);

            socketIn = new DataInputStream(connection.getInputStream()); // Read data from server
            socketOut = new DataOutputStream(connection.getOutputStream()); // Write data to server

            socketOut.writeUTF(filename); // Write filename to server
            
            // Read file contents from server
            while (true) {
                bytes = socketIn.read(buffer, 0, BUFFER_SIZE); // Read from socket
                if (bytes <= 0) break; // Check for end of file
                System.out.print(new String(buffer, StandardCharsets.UTF_8)); // Write to standard output
            }

            connection.close();
        } catch (Exception ex) {
            System.out.println("Error: " + ex);
        }
    }

    public static void main(String[] args) {
        Client client = new Client("127.0.0.1", 5000, args[0]);
    }
}
