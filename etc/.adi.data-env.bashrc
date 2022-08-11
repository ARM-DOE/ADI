#------------------------------------------------------------------------------
# Set Standard Data Directories
#
# Set DATA_HOME before sourcing this file:
#
# export DATA_HOME=/home/$USER/data
#------------------------------------------------------------------------------

if [ -z "$DATA_HOME" ]; then
   export DATA_HOME=/home/$USER/data
fi

export COLLECTION_DATA=$DATA_HOME/collection
export CONF_DATA=$DATA_HOME/conf
export DATASTREAM_DATA=$DATA_HOME/datastream
export LOGS_DATA=$DATA_HOME/logs
export TMP_DATA=$DATA_HOME/tmp
export WWW_DATA=$DATA_HOME/www
export QUICKLOOK_DATA=$WWW_DATA/process

