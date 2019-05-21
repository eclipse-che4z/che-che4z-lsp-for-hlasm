import com.broadcom.HlasmPlugin.HlasmPlugin;
import com.broadcom.HlasmPlugin.Diagnostic;

public class JNITest
{
	static
	{
        System.loadLibrary("jni_parser_library");                       
    }
 
    
    

	public static void main(String[] args)
	{
		HlasmPlugin test = new HlasmPlugin();
		Diagnostic[] diags = test.parseFile("any/path/to/file", " LR 1,1");

		if(diags.length != 0)
		{
			System.out.println("The number of diagnostics for \" LR 1,1\" is greater than 0.");
			System.exit(1);
		}

		diags = test.parseFile("any/path/to/second/file", " LR 1,");

		if(diags.length <= 0)
		{
			System.out.println("The number of diagnostics for \" LR 1,\" is not greater than 0.");
			System.exit(1);
		}
		
		for(int i = 0; i < diags.length; ++i)
		{
			Diagnostic d = diags[i];
			if(!d.getFileName().equals("any/path/to/second/file"))
			{
				System.out.println("Wrong FileName of diagnostic: expected \"any/path/to/second/file\", instead got \"" + d.getFileName() + "\"");
				System.exit(1);
			}
			
			System.out.println(d.getFileName() + " { [" + Integer.toString(d.getBeginLine()) + "," + Integer.toString(d.getBeginCol()) + "],[" + Integer.toString(d.getEndLine()) + "," + Integer.toString(d.getEndCol()) + "] } " + d.getSource() + " " + Integer.toString(d.getSeverity()) + " " + d.getMessage());
		}

		System.exit(0);
	}
}
