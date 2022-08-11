#------------------------------------------------------------------------------
# Set Standard Data Directories
#
# Set DATA_HOME before sourcing this file:
#
# setenv DATA_HOME /home/$USER/data
#------------------------------------------------------------------------------

if ( ! $?DATA_HOME ) then
   setenv DATA_HOME /home/$USER/data
endif

setenv COLLECTION_DATA     $DATA_HOME/collection
setenv CONF_DATA           $DATA_HOME/conf
setenv DATASTREAM_DATA     $DATA_HOME/datastream
setenv LOGS_DATA           $DATA_HOME/logs
setenv TMP_DATA            $DATA_HOME/tmp
setenv WWW_DATA            $DATA_HOME/www
setenv QUICKLOOK_DATA      $WWW_DATA/process

