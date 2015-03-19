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
      if ($json && isset($json->error) && $json->error->message == 'Access denied') {
        $migration = Migration::currentMigration();
        $message = t('Access denied for !objecturl', array('!objecturl' => $item_url));
        $migration->getMap()->saveMessage(array($id), $message, MigrationBase::MESSAGE_ERROR);
        $result = $this->emptyItem($id);
        return $result;
      }
    }
    else {
      $json = json_decode(file_get_contents($item_url));
      return $json->result;
    }
    $migration = Migration::currentMigration();
    $message = t('Loading of !objecturl failed:', array('!objecturl' => $item_url));
    $migration->getMap()->saveMessage(array($id), $message, MigrationBase::MESSAGE_ERROR);
    return new stdClass();
  }

  /**
   * Creates a stub entry.
   */
  public function emptyItem($id) {
    $result = new stdClass();
    $result->id = substr($id, 0, 35);
    $result->title = t('Access denied for %id', array('%id' => $id));
    $result->name = $id;
    return $result;
  }
}

abstract class MigrateCkanBase extends Migration {
  /**
   * Here we go.
   */
  public function __construct($arguments) {
    $this->endpoint = isset($arguments['endpoint']) ? $arguments['endpoint'] : 'http://demo.getdkan.com/api/3/action/';
    parent::__construct($arguments);
  }

  /**
   * Looks up format if exists, if not creates it.
   */
  public function createTax($name, $vocab_name, $vid) {
    if (!taxonomy_get_term_by_name($name, $vocab_name, $vid)) {
      $new_term = new stdClass();
      $new_term->name = $name;
      $new_term->vid = $vid;
      taxonomy_term_save($new_term);
    }
  }

  /**
   * Converts times with edge cases.
   */
  public function StringToTime($date) {
    if ($date = str_replace('-', '', $date)) {
      return strtotime($date);
    }
    else {
      return strtotime($date);
    }
  }

  /**
   * Saves file locally.
   *
   * @param string $url
   *   Full url to file to download
   * @param string $uri
   *   Drupal uri of where to save file public://archive/test.pdf
   * @param int $save_mode
   *   File save mode from drupal core, ex FILE_EXISTS_REPLACE
   */
  public function downloadExternalFile($url, $uri, $save_mode = FILE_EXISTS_RENAME, $manage_file = TRUE) {

    $url_info = parse_url($url);
    $url_path_info = pathinfo($url_info['path']);

    // This helps with filenames with spaces.
    $url = $url_info['scheme'] . '://' . $url_info['host']  . $url_path_info['dirname'] . '/' . rawurlencode($url_path_info['basename']);

    // Need to remove the filename from the uri.
    $uri_target = file_uri_target($uri);
    $uri_scheme = file_uri_scheme($uri);
    $uri_path_info = pathinfo($uri_target);
    $directory = file_stream_wrapper_uri_normalize($uri_scheme . "://" . $uri_path_info['dirname']);

    if (file_prepare_directory($directory, FILE_CREATE_DIRECTORY)) {
      $drupal_result = drupal_http_request($url);
      if (!empty($drupal_result->data)) {
        $path = file_stream_wrapper_uri_normalize($uri);
        if ($manage_file) {
          $new_file = file_save_data($drupal_result->data, $path, $save_mode);
        }
        else {
          return file_unmanaged_save_data($drupal_result->data, $path, $save_mode);
        }
      }
      else {
        drupal_set_message("Error downloading file, no data recieved for " . $url);
        return FALSE;
      }
      $new_file->display = 1;
      return (array) $new_file;
    }
    else {
      drupal_set_message(t("Could not create directory"));
    }
  }

}
