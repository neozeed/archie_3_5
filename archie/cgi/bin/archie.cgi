#!/usr/local/bin/perl
#
# Archie Perl Client (front-end)
# This script needs the binary cgi-client (back-end) to pass the query parameters 
# to. It then reads the result returned from cgi-client and reformats it
# for the http server.
##############################################################################

##############################################################################
# These few lines MUST be configured according to your system.
# The first variable holds the full path to your cgi-client binary which could 
# be of any name you wish. The $archie_user variable holds the user name under which 
# your archie service is running.
##############################################################################

$archiebin = "/work/archie/cgi-client";
   # You can rename the binary to archie.bin or any other suitable name you
   # choose.

$archieperl = "http://archie.bunyip.com/cgi-bin/archie.cgi";
   # This same perl script you are looking at. Where will its URL be?

$archie_user = "archie";
   # If your archie server is running under another user name then please
   # change this accordingly.

$adv_page = "http://archie.bunyip.com/archie-adv.html";
$smpl_page = "http://archie.bunyip.com/archie.html";
   # The above URLs will take you to our web pages!  We don't mind but we doubt
   # that you setup an Archie server for nothing!  Please change the web site
   # in those lines to your web site name.

$gif_url = "http://archie.bunyip.com/results.gif";
   # Where the results.gif file is.
##############################################################################
# The following lines hold the different strings sent back to the httpd
# server.  It is optional to modify them to comply with your HTML-look 
# preference.
##############################################################################

$excerpt_offset = 9;
$site_offset = 6;
$info_offset = 5;

$archie_title = "<HTML><TITLE>Archie Results</TITLE>\n";
$archie_header_plain = "<H2>Archie Results</H2><PRE>\n";
$archie_header_logo = "<BODY BGCOLOR=\"#FFFFFF\"><IMG SRC=\"%s\" ALT=\"Archie Results\">\n";
$archie_continue_button = "<CENTER><B><A HREF=\"$adv_page\">New Advanced Query</A> | <A HREF=\"$smpl_page\">New Simple Query</A><br><FONT SIZE=\"+2\">M</FONT>odify <FONT SIZE=\"+2\">S</FONT>earch</B> : <INPUT TYPE=text SIZE=40 NAME=query VALUE=\"%s\"><br></CENTER>\n";
$archie_pages = "<CENTER><B><A HREF=\"$adv_page\">New Advanced Query</A> | <A HREF=\"$smpl_page\">New Simple Query</A><br></CENTER>\n";
$str1 = "<h3>Found %d Hit(s)</h3>\n";
$str2 = "<i>(%d)</i><b> %s </b><p>\n";
$str3 = "<INPUT TYPE=\"submit\" VALUE=\"Search for More\">\n";
$str4 = "<INPUT TYPE=hidden NAME=\"%s\" VALUE=\"%s\">\n";
$str5 = "<i>(%d)</i><B>%s</B><p>\n";
$str6 = "<i>(%d)</i><B><A HREF=\"%s\">%s</A></B><p>\n";
$str7 = ' 'x$info_offset."<A HREF=\"%s://%s%s\"><i>%d </i></A> ";
$str8 = ' 'x$site_offset."<b><A HREF=\"%s\">%s</A></b>";
$str9 = "<b>Keyword: </b>%s";
$str10 = "<b>Date: </b>%s";
$str11 = ' 'x$info_offset."<b>Size: </b>%d";
$str12 = ' 'x$excerpt_offset."</pre><BLOCKQUOTE><i>%s</i></BLOCKQUOTE> <p> <pre>";
$str13 = "</FORM></PRE></BODY></HTML>\n";
$str14 = "<b>Type: </b>%s";

format OUTINFO =
~      </b><i>@<<<<<<<<<<<<<@<<<<<<<<<<< @<<<<<<<<<<<<<<<<<<<<<<<<<<< </i>@*
$perms, $size, $date, $strout
.


format OUTPATH =
<b>@*
$path
.

##############################################################################
# END OF CHANGES
##############################################################################

%proto =(
"Anonymous FTP", "ftp",
"Web Index", "http"
);

%main_tran =(
 FORM_PLAIN_OUTPUT_FLAG, "oflag",
 FORM_CASE, "case",
 FORM_CASE_SENS, "Sensitive",
 FORM_CASE_INS, "Insensitive",
 FORM_QUERY, "query",
 FORM_ORIG_QUERY, "origquery",
 FORM_OLD_QUERY, "oldquery",
 FORM_STRINGS_ONLY, "strings",
 FORM_MORE_SEARCH, "more",
 FORM_SERV_URL, "url",
 FORM_GIF_URL, "gifurl",
 FORM_STR_HANDLE, "strhan",
 FORM_STRINGS_NO, "NO",
 FORM_STRINGS_YES, "YES",
 FORM_DB, "database",
 FORM_ANONFTP_DB, "Anonymous FTP",
 FORM_WEB_DB, "Web Index",
 I_ANONFTP_DB, 0,
 I_WEBINDEX_DB, 1,
 FORM_TYPE, "type",
 FORM_EXACT, "Exact",
 FORM_SUB, "Sub String",
 FORM_REGEX, "Regular Expression",
 FORM_MAX_HITS, "maxhits",
 FORM_MAX_HPM, "maxhpm",
 FORM_MAX_MATCH, "maxmatch",
 FORM_PATH_REL, "pathrel",
 FORM_PATH, "path",
 FORM_EXCLUDE_PATH, "expath",
 FORM_AND, "AND",
 FORM_OR, "OR",
 FORM_START_STRING, "start_string",
 FORM_START_STOP, "start_stop",
 FORM_START_SITE_STOP, "start_site_stop",
 FORM_START_SITE_FILE, "start_site_file",
 FORM_START_SITE_PRNT, "start_site_prnt",
 FORM_BOOLEAN_MORE_ENT, "bool_more_ent",
 PATH_AND, 0,
 PATH_OR, 1,
 FORM_DOMAINS, "domains",
 FORM_DOMAIN_1, "domain1",
 FORM_DOMAIN_2, "domain2",
 FORM_DOMAIN_3, "domain3",
 FORM_DOMAIN_4, "domain4",
 FORM_DOMAIN_5, "domain5",
 PLAIN_HITS, "HITS",
 FORM_ERROR, "ERROR",
 FORM_FORMAT, "format",
 FORM_FORMAT_KEYS, "Keywords Only",
 FORM_FORMAT_EXC, "Excerpts Only",
 FORM_FORMAT_LINKS, "Links Only",
 FORM_FORMAT_STRINGS_ONLY, "Strings Only",
 FTYPE_NOT_INDX, "Not indexed yet",
 FTYPE_UNINDX, "Unindexable",
 FTYPE_INDX, "indexed",
 I_FORMAT_KEYS,  2,
 I_FORMAT_EXC,  0,
 I_FORMAT_LINKS,  1
);

%result_tran=(
 PLAIN_START, "START_RESULT",
 PLAIN_END, "END_RESULT",
 PLAIN_URL, "URL",
 PLAIN_STRING, "STRING",
 PLAIN_TITLE, "TITLE",
 PLAIN_NO_TITLE, "NO_TITLE",
 PLAIN_SITE, "SITE",
 PLAIN_PATH, "PATH",
 PLAIN_TYPE, "TYPE",
 PLAIN_WEIGHT, "WEIGHT",
 PLAIN_TEXT, "TEXT",
 PLAIN_PERMS, "PERMS",
 PLAIN_SIZE, "SIZE",
 PLAIN_DATE, "DATE",
 PLAIN_FILE, "FILE",
 PLAIN_KEY, "KEY",
 PLAIN_START_STRINGS, "START_STRINGS_ONLY",
 PLAIN_END_STRINGS, "END_STRINGS_ONLY",
 PLAIN_FTYPE, "FTYPE",
 PLAIN_FTYPE_INDX, "FTYPE_INDX",
 PLAIN_FTYPE_NOT_INDX, "FTYPE_NOT_INDX",
 PLAIN_FTYPE_UNINDX, "FTYPE_UNINDX"
);

$ENV{"ARCH_USER"} = $archie_user;
$| = 1;
open(stdin,"-");
open(IN,"$archiebin < '$stdin' |");

$i = 1;
$s = 1;
$top=1;
$k = 1;
$no_sites = 1;
$entry{"FORM_GIF_URL"} = $gif_url;
$entry{"FORM_SERV_URL"} = $archieperl;
while (<IN>){
    chop;
    if( $_ =~ /^$main_tran{"FORM_PLAIN_OUTPUT_FLAG"}/ ){
				($junk,$flag) = split(/=/,$_,2);
				if( $flag == 1 ){
						## This is for plain results returned to the web
						## server in record-like format (no processing is 
						## done on the results). Used for certain types
						## of URAs. Unlikely to be used by our clients!
						print STDOUT "Content-type: text/plain\n\n";
						print STDOUT $_."\n";
						foreach $k (keys %entry){
								if( $k ne "FORM_QUERY" ){
										print STDOUT "$main_tran{$k}=$entry{$k}\n";
								}
						}
						while (<IN>){
								print STDOUT $_;
						}
						exit 1;
				}
    }
    if ( $_ =~ /^$main_tran{"FORM_ERROR"}/ ) {
 				$entry{"PLAIN_HITS"} = 0;
				($junk,$err) = split(/=/,$_,2);
				&print_error($err,$entry{"FORM_GIF_URL"});
				last;
    }elsif( ($i == 0) && !( $_ =~ /^$main_tran{"PLAIN_HITS"}/ )){
				&print_error("Returned results are incorrect",$entry{"FORM_GIF_URL"});
				last;
    }else{
				if( !($_ =~ /=/) ) {
						last;
				}
				($junk,$value) = split(/=/,$_,2);
				if( ($i == 1) && ( $junk eq $main_tran{"PLAIN_HITS"} ) && ($value == 0)){
						&print_error("No hits",$entry{"FORM_GIF_URL"});
						exit 1;
				}
				if($value eq ""){
						next;
				}
				$main = 0;
				foreach $key (keys (%main_tran)) {
						if( $main_tran{$key} eq $junk ){
								if( $key eq "FORM_ORIG_QUERY" ){
										$value =~ s/"/&quot\;/g;
                }
								$entry{$key} = $value;
								$main = 1;
								last;
						}
				}
				$strings_only=0;
				if($result_tran{"PLAIN_START_STRINGS"} eq $junk){
						$strings_only = 1;
						&print_header($entry{"FORM_GIF_URL"},$entry{"FORM_SERV_URL"});
						printf STDOUT $str1,$entry{"PLAIN_HITS"};
						$_ = <IN>;
						if( $_ eq "" ){
								last;
						}
						chop;
						if( $_ !~ /=/ ) {
								last;
						}
						($junk,$value) = split(/=/,$_,2);
						if($value eq ""){
								next;
						}
				}elsif( ($main == 0) && ($junk ne $result_tran{"PLAIN_START"}) ){
						$entry{$junk} = $value;
						$main = 1;
				}

        if( $main == 0 ){
						if( defined($entry{"FORM_DB"}) && ($entry{"FORM_DB"} eq $main_tran{"FORM_ANONFTP_DB"} )){
								$db = $main_tran{"I_ANONFTP_DB"};
						}else{
								$db = $main_tran{"I_WEBINDEX_DB"};
						}
						if (($strings_only == 0) && (!defined( $entry{"FORM_SERV_URL"} )) ){
								&print_error("The submitted information does not include URL information. Read the HELP pages to setup your html page correctly.",$entry{"FORM_GIF_URL"});
								exit 1;
						}
# do while instead

						while (1){
								if($result_tran{"PLAIN_END_STRINGS"} eq $junk){
                    last;
								}elsif($strings_only == 1){
                    printf STDOUT $str2, $s ,$value;
										$s++;
								}elsif( $result_tran{"PLAIN_START"} eq $junk ){
										$rec_num = $value+1;
										if($top != 1){
												$old_site = $curr_site;
												$old_str = $curr_str;
										}
								}elsif( $result_tran{"PLAIN_END"} eq $junk ){
										if ($top == 1){
												&print_header($entry{"FORM_GIF_URL"},$entry{"FORM_SERV_URL"});
												printf STDOUT $archie_continue_button, $entry{"FORM_ORIG_QUERY"};
												foreach $k ( keys %entry ){
														if( $k eq "PLAIN_HITS" ){
																printf STDOUT $str1, $entry{$k};
																printf STDOUT $str3;
														}else{
                                if( $k ne "FORM_QUERY" ){
																  printf STDOUT $str4, $main_tran{$k}, $entry{$k};
                                }
														}
												}
												print STDOUT "<PRE>\n";
												$top=0;
										}
										if($db == $main_tran{"I_ANONFTP_DB"}){
												if( $old_site ne $curr_site ){
														## Do some processing for a new site
														$no_sites++;
														printf STDOUT "<p>";

														if($result{"PLAIN_STRING"} ne $result{"PLAIN_SITE"}){
																printf STDOUT $str5, $rec_num, $result{"PLAIN_SITE"};
														}else{
																printf STDOUT $str6, $rec_num, $result{"PLAIN_URL"}, $result{"PLAIN_SITE"};
														}
												}
												$str = $result{"PLAIN_STRING"};
												$size = $result{"PLAIN_SIZE"};
												$date = $result{"PLAIN_DATE"};
												$perms = $result{"PLAIN_PERMS"};
												if($result{"PLAIN_STRING"} ne $result{"PLAIN_SITE"}){
														$path = $result{"PLAIN_PATH"};
														&print_spaces($info_offset);
														printf STDOUT $str7, $proto{"Anonymous FTP"}, $result{"PLAIN_SITE"}, $result{"PLAIN_PATH"}, $rec_num;

														$~ = "OUTPATH";
														write;

														$strout = "<A HREF=\"".$result{"PLAIN_URL"}."\">".$result{"PLAIN_STRING"}."</A><p>";
														$~ = "OUTINFO";
														write;
												}
										}elsif($db == $main_tran{"I_WEBINDEX_DB"}){
												if( defined( $result{"PLAIN_TITLE"})){
														$title =$result{"PLAIN_TITLE"};
												}else{
														$title = $result{"PLAIN_NO_TITLE"};
												}

												(defined ($result{"PLAIN_SITE"})) && ($site = $result{"PLAIN_SITE"});
												(defined ($result{"PLAIN_TEXT"})) && ($text = $result{"PLAIN_TEXT"});
												(defined ($result{"PLAIN_STRING"})) && ($str = $result{"PLAIN_STRING"});
												(defined ($result{"PLAIN_SIZE"})) && ($size = $result{"PLAIN_SIZE"});
												(defined ($result{"PLAIN_WEIGHT"})) && ($weight = $result{"PLAIN_WEIGHT"});
												(defined ($result{"PLAIN_TYPE"})) && ($type = $result{"PLAIN_TYPE"});
												(defined ($result{"PLAIN_DATE"})) && ($date = $result{"PLAIN_DATE"});
												(defined ($result{"PLAIN_URL"})) && ($strurl = $result{"PLAIN_URL"});
												(defined ($result{"PLAIN_FTYPE"})) && ($ftype = $result{"PLAIN_FTYPE"});
												if ($strurl eq $title) {
														$title =~ s/:80\//\//;
														$strurl = $title;
														$title = "(NO-TITLE)    <i>$title</i>";
												}elsif($strurl !~ /:80\d+/){
														$strurl =~ s/:80//;
												}
												if($site !~ /:80\d+/){
														$site =~ s/:80//;
												}
												if( $old_site ne $curr_site ){
														## Do some processing for a new site
														$no_sites++;
														print STDOUT "<p>";

														if($result{"PLAIN_STRING"} ne $result{"PLAIN_SITE"}){
																printf STDOUT $str5, $rec_num, $site;
														}else{
																printf STDOUT $str6, $rec_num, $strurl, $site;
														}
												}

												if($result{"PLAIN_STRING"} ne $result{"PLAIN_SITE"}){
														&print_spaces($site_offset);
														printf STDOUT $str8, $strurl, $title;
                            defined( $ftype ) && ($ftype ne $result_tran{"PLAIN_FTYPE_INDX"}) && (print STDOUT "\n\n".' 'x$site_offset) && (printf STDOUT $str14, $main_tran{$result{"PLAIN_FTYPE"}} );
#														defined( $weight ) && (print STDOUT "\n\n".' 'x$site_offset) && (printf STDOUT $str9, $str);
														defined( $type ) && ( $type eq $result_tran{"PLAIN_KEY"}) && (print STDOUT "\n\n".' 'x$site_offset) && (printf STDOUT $str9, $str);
														defined( $date ) && (print STDOUT "\n\n".' 'x$site_offset) && (printf STDOUT $str10, $date) && (&print_spaces($info_offset)) && (printf STDOUT $str11, $size);
														printf STDOUT $str12, $text;
												}
										}

										undef(%result);
										undef $site; undef $text; undef $str; undef $size; undef $weight; undef $date; undef $strurl;
								}else{
										foreach $key (keys (%result_tran)) {
												if( $result_tran{$key} eq $junk ){
														$result{$key} = $value;
														if($key eq "PLAIN_SITE"){
																$curr_site = $value;
														}
														if($key eq "PLAIN_STRING"){
																$curr_str = $value;
														}
														$main = 1;
														last;
												}
										}
								}
                $_ = <IN>;
								if( $_ eq "" ){
										last;
								}
								chop;
								if( $_ !~ /=/ ) {
										last;
								}
								($junk,$value) = split(/=/,$_,2);
								if($value eq ""){
										next;
								}
						}
						last;
				}
    }
    $i++;
}
# printf STDOUT $archie_continue_button, $entry{"FORM_OLD_QUERY"};
if( !defined( $entry{"PLAIN_HITS"} ) ){
 		&print_error("No hits",$entry{"FORM_GIF_URL"});
}elsif($err eq ""){
 		print STDOUT $str13;
}
exit 1;



sub print_error {
		local($error_msg,$gifurl) = @_;
		print STDOUT "Content-type: text/html\n\n";
		if( $gifurl eq "" ){
				print STDOUT "<html><center><h2>Archie Results</h2><HR>\n\n";
		}else{
				printf STDOUT $archie_header_logo, $gifurl;
				print STDOUT "<html><center><h2>Archie Results</h2><HR>\n\n";
		}
#  print STDOUT "<html><h2>Archie Error</h2><HR>\n\n";

		print STDOUT "<i>".$error_msg."</i></center><br>\n";
		printf STDOUT $archie_pages;
		print STDOUT "</html>\n";
		return(1);
}

sub print_header {
  local($gifurl, $url) = @_;
  print STDOUT "Content-type: text/html\n\n";
  printf STDOUT $archie_title;
  if( $gifurl eq "" ){
			printf STDOUT $archie_header_plain;
	}else{
			printf STDOUT $archie_header_logo, $gifurl;
	}
  print STDOUT "<FORM method=\"POST\" action=\"".$url."\">\n";
  return(1);
}

sub print_spaces {
		local($n) = $_;
    print STDOUT ' 'x$n;
		return(1);
}
