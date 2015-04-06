<?php

class DKANMigrateBaseValidator
{
  public function resourceList($list)
  {
        $list = json_decode($list);
        return $list->result;
  }
  public function resourceListCount($list)
  {
        $list = json_decode($list);
        return count($list->result);
  }

  public function getFile($file)
  {
        return file_get_contents($file);
  }
}
