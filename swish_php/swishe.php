<?
if(!extension_loaded('swishe')) {
	dl('./swishe.so');
}
$module = 'swishe';
$functions = get_extension_funcs($module);
echo "Functions available in the test extension:<br>\n";
foreach($functions as $func) {
    echo $func."<br>\n";
}
echo "<br>\n";
$function = 'confirm_' . $module . '_compiled';
if (extension_loaded($module)) {
	$str = $function($module);
} else {
	$str = "Module $module is not compiled into PHP";
}
echo "$str\n";

$index_file = '/root/swish-e/tests/index.swish-e';
$swish = swishe($index_file);

if(($err = $swish->error()))
{
    echo "ERROR: $err (should be 0 on success)\n";
    $str_error = $swish->error_string();
    echo "ERROR TEXT: $str_error\n";
    $crit_error = $swish->critical_error();
    echo "ERROR CRITICAL: $crit_error\n";
}

$word='morfing';
$stem = $swish->stem($word);
echo "$word is stemmed as $stem\n";

echo "WORDS:";
$words = $swish->swish_words_by_letter($index_file,'t');
foreach ($words as $val)
{
   echo " $val";
}
echo "\n";

echo "INDEX_NAMES:";
$index_names = $swish->index_names();
foreach ($index_names as $val)
{
  echo " $val";
}
echo "\n";

echo "HEADER_NAMES AND HEADER_VALUES:\n";
$header_names = $swish->header_names();
foreach ($header_names as $val)
{
  $data = $swish->header_value($index_file,$val);
  echo "-->$val $data\n";
}

//$results = $swish->query("test");
$search = $swish->new_search_object("test");
//$search->set_query("test");
//$search->set_search_limit('fpu','20020101','20020131');
$search->set_sort('swishdocpath asc swishrank desc');
$results = $search->execute();


$parsed_words = $results->parsed_words($index_file);
echo "Parsed words:\n";
foreach ($parsed_words as $val)
{
    echo "    $val\n";
}

$hits = $results->hits();
echo "HITS: $hits\n";

for($i = 0; $i < $hits; $i++)
{
    $result = $results->next_result();
    $rank = $result->property('swishrank');
    $dbfile = $result->property('swishdbfile');
    $path = $result->property('swishdocpath');
    $size = $result->property('swishdocsize');
    $total_words = $result->result_index_value("Total Words");
    echo "$rank $dbfile $path $size\n";
}
?>
