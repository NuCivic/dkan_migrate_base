<?php

/**
 * @file
 * Migration file for dkan_migrate_base.
 */

/**
 * Implements hook_migrate_api().
 */
function dkan_migrate_base_migrate_api() {
  $api = array(
    // Migrate API, not CKAN's of course.
    'api' => 2,
    'groups' => array(
      'dkan' => array(
        'title' => t('DKAN'),
      ),
    ),
    'migrations' => array(
      'ckan_dataset_base' => array(
        'class_name' => 'MigrateCkanDatasetBase',
        'group_name' => 'dkan',
        'title' => t('DKAN Dataset Base'),
      ),
      'ckan_group_base' => array(
        'class_name' => 'MigrateCkanGroupBase',
        'group_name' => 'dkan',
        'title' => t('DKAN Group Base'),
      ),
      'ckan_resource_base' => array(
        'class_name' => 'MigrateCkanResourceBase',
        'group_name' => 'dkan',
        'title' => t('DKAN Resource Base'),
      ),
    ),
  );
  return $api;
}

class CKANListJSON extends MigrateListJSON {
  /**
   * The default implementation assumes the IDs are top-level array elements.
   */
  protected function getIDsFromJSON(array $data) {
    $ids = array();
    foreach ($data['result'] as $item) {
      $ids[] = $item;
    }
    return $ids;
  }

  /**
   * Implements computeCount().
   */
  public function computeCount() {
    $count = 0;
    if (empty($this->httpOptions)) {
      $json = file_get_contents($this->listUrl);
    }
    else {
      $response = drupal_http_request($this->listUrl, $this->httpOptions);
      $json = $response->data;
    }
    if ($json) {
      $data = drupal_json_decode($json);
      if ($data) {
        $count = count($data['result']);
      }
    }
    return $count;
  }
}

class CKANItemJSON extends MigrateItemJSON {

  /**
   * Parses for 'results' instead of base.
   */
  protected function getIDsFromJSON(array $data) {
    $ids = array();
    foreach ($data['result'] as $item) {
      $ids[] = $item;
    }
    return $ids;
  }
  /**
   * Parses for 'results' instead of base.
   */
  public function computeCount() {
    $count = 0;
    if (empty($this->httpOptions)) {
      $json = file_get_contents($this->listUrl);
    }
    else {
      $response = drupal_http_request($this->listUrl, $this->httpOptions);
      $json = $response->data;
    }
    if ($json) {
      $data = drupal_json_decode($json);
      if ($data) {
        $count = count($data['result']);
      }
    }
    return $count;
  }

  /**
   * Implementors are expected to return an object representing a source item.
   */
  public function getItem($id) {
    $item_url = $this->constructItemUrl($id);
    if (filter_var($item_url, FILTER_VALIDATE_URL)) {
      // Get the JSON object at the specified URL.
      $json = $this->loadJSONUrl($item_url);
      if ($json && isset($json->result)) {
        return $json->result;
      }
    }
    else {
      $json = json_decode(file_get_contents($item_url));
      return $json->result;
    }
    $migration = Migration::currentMigration();
    $message = t('Loading of !objecturl failed:', array('!objecturl' => $item_url));
    $migration->getMap()->saveMessage(
    array($id), $message, MigrationBase::MESSAGE_ERROR);
    return NULL;
  }

}

abstract class MigrateCkanBase extends Migration {
  /**
   * Here we go.
   */
  public function __construct($arguments) {
    $this->endpoint = isset($arguments['endpoint']) ? $arguments['endpoint'] : 'http://demo.ckan.org/api/3/action/';
    parent::__construct($arguments);
  }
}
