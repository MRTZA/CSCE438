/* 
* Reference: 
* http://geekdirt.com/blog/introduction-to-hadoop/
*/

import java.io.IOException;
import java.util.StringTokenizer;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.conf.Configured;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.IntWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.input.NLineInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;
import org.apache.hadoop.util.Tool;
import org.apache.hadoop.util.ToolRunner;

import java.util.regex.Pattern;
import java.util.regex.Matcher;

public class WordCount {

  public static boolean isTime(String input) {
    Pattern p = Pattern.compile("^((?:[01]\\d|2[0-3]):[0-5]\\d:[0-5]\\d$)");
    Matcher m = p.matcher(input);
    if(m.matches()) {
        return true;
    } else {
        return false;
    }
  }

  public static String getBin(String input) {
      String result = "";
      
      String bin = input.substring(0, 2);

      result += bin + ":00 - " + bin + ":59";
      
      return result;
  }

  public static class TokenizerMapper
       extends Mapper<Object, Text, Text, IntWritable>{

    private final static IntWritable one = new IntWritable(1);
    private Text word = new Text();
    private String curr_time = "-10";
    private boolean isNewTweet = true;

    public void map(Object key, Text value, Context context
                    ) throws IOException, InterruptedException {
      StringTokenizer itr = new StringTokenizer(value.toString());

      while (itr.hasMoreTokens()) {
        String lastToken = itr.nextToken(); 
        if(isTime(lastToken)) {
            curr_time = lastToken;
            isNewTweet = true;
        }
        else if(lastToken.equalsIgnoreCase("sleep") && isNewTweet) {
            word.set(getBin(curr_time));
            context.write(word, one);
            isNewTweet = false;
        }   
      }
    }
  }

  public static class IntSumReducer
       extends Reducer<Text,IntWritable,Text,IntWritable> {
    private IntWritable result = new IntWritable();

    public void reduce(Text key, Iterable<IntWritable> values,
                       Context context
                       ) throws IOException, InterruptedException {
      int sum = 0;
      for (IntWritable val : values) {
        sum += val.get();
      }
      result.set(sum);
      context.write(key, result);
    }
  }

  public static void main(String[] args) throws Exception {
    Configuration conf = new Configuration();
    Job job = Job.getInstance(conf, "word count");
    job.setJarByClass(WordCount.class);
    job.setMapperClass(TokenizerMapper.class);
    job.setCombinerClass(IntSumReducer.class);
    job.setReducerClass(IntSumReducer.class);
    job.setOutputKeyClass(Text.class);
    job.setOutputValueClass(IntWritable.class);
    FileInputFormat.addInputPath(job, new Path(args[0]));
    FileOutputFormat.setOutputPath(job, new Path(args[1]));
    System.exit(job.waitForCompletion(true) ? 0 : 1);
  }
}