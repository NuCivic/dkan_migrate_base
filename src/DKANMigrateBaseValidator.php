<?php

class DKANMigrateBaseValidator
{
    public function dkan_migrate_base_create_resource_list_items($endpoint, $fileName)
    {
          return dkan_migrate_base_create_resource_list_items($endpoint, $fileName);
    }

    public function getFile($file)
    {
          return file_get_contents($file);
    }
}
