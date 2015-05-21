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
    'api'        => 2,
    'groups'     => array(
      'dkan' => array(
        'title' => t('DKAN'),
      ),
    ),
    'migrations' => array(
      'ckan_dataset_base'  => array(
        'class_name' => 'MigrateCkanDatasetBase',
        'group_name' => 'dkan',
        'title'      => t('CKAN Dataset Base'),
      ),
      'ckan_group_base'    => array(
        'class_name' => 'MigrateCkanGroupBase',
        'group_name' => 'dkan',
        'title'      => t('CKAN Group Base'),
      ),
      'ckan_resource_base' => array(
        'class_name' => 'MigrateCkanResourceBase',
        'group_name' => 'dkan',
        'title'      => t('CKAN Resource Base'),
      ),
    ),
  );

  if (FALSE === node_type_get_type('organization')) {
    drupal_set_message(t("Organization content type not detected. MigrateCkanOrganizationBase migration not registered."));
  }
  else {
    $api['migrations']['ckan_organization_base'] = array(
      'class_name' => 'MigrateCkanOrganizationBase',
      'group_name' => 'dkan',
      'title'      => t('CKAN Organization Base'),
    );
  }

  return $api;
}

class CKANListJSON extends MigrateListJSON {
  /**
   * Initialiazes.
   */
  public function __construct($list_url, $http_options = array()) {
    parent::__construct($list_url);
    $this->httpOptions = $http_options;

    // WAS: How many elements to retrieve. This parameter has been commented out, I couldn't find any documentation nor any reference of it in the code.
    // $this->page = isset($http_options['page']) ? $http_options['page'] : '';

    // The starting offset (by default start from the first element).
    $this->offset = isset($http_options['offset']) && is_numeric($http_options['offset']) ? $http_options['offset'] : 0;

    // In order to support mtm (multi-threaded migrate) we need to get the *limit* parameter (NULL equals no limit).
    $this->limit = isset($http_options['limit']) && is_numeric($http_options['limit']) ? $http_options['limit'] : NULL;
  }

  /**
   * The default implementation assumes the IDs are top-level array elements.
   */
  protected function getIDsFromJSON(array $data) {

    // Get the portion of results within the specified boundaries (starting from *offset*).
    $ids = array_slice($data['result'], $this->offset, $this->limit);

    $this->log("returning " . sizeof($ids) . " items [ offset :: {$this->offset} ][ limit :: " . (isset($this->limit) ? $this->limit : 'not set') . " ]");

    return $ids;
  }

  /**
   * Loads the JSON at the given URL, and returns an array of the IDs.
   *
   * @return array
   *   Array of IDs.
   */
  public function getIdList() {
    migrate_instrument_start("Retrieve $this->listUrl");

    if (substr($this->listUrl, 0, 6) == 'public') {
      $json = file_get_contents($this->listUrl);
    }
    else {
      $response = drupal_http_request($this->listUrl, $this->httpOptions);
      $json     = $response->data;
    }
    migrate_instrument_stop("Retrieve $this->listUrl");
    if ($json) {
      $data = drupal_json_decode($json);
      if ($data) {
        return $this->getIDsFromJSON($data);
      }
    }
    Migration::displayMessage(t('Loading of !listurl failed:', array('!listurl' => $this->listUrl)));
    return NULL;
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
      $json     = $response->data;
    }
    if ($json) {
      $data = drupal_json_decode($json);
      if ($data) {
        $count = count($data['result']);
      }
    }

    // If a limit has been set and the count is larger then that, we return the limit.
    if (isset($this->limit) && $count > $this->limit) {
      $count = $this->limit;
    }
    return $count;
  }

  /**
   * Log a message.
   *
   * @param $message
   */
  private function log($message) {

    Migration::displayMessage($message);

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
      $json     = $response->data;
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
        $message   = t('Access denied for !objecturl', array('!objecturl' => $item_url));
        $migration->getMap()
                  ->saveMessage(array($id), $message, MigrationBase::MESSAGE_ERROR);
        $result = $this->emptyItem($id);
        return $result;
      }
    }
    else {
      $json = json_decode(file_get_contents($item_url));
      return $json->result;
    }
    $migration = Migration::currentMigration();
    $message   = t('Loading of !objecturl failed:', array('!objecturl' => $item_url));
    $migration->getMap()
              ->saveMessage(array($id), $message, MigrationBase::MESSAGE_ERROR);
    return new stdClass();
  }

  /**
   * Override the superclass loadJSONUrl in order to use the standard {@link drupal_http_request}
   * and retry if the remote system returns an error. Current settings are to
   * retry every 5 secs. for a total of 5 times top.
   *
   * @param string $item_url The item URL.
   *
   * @return mixed The object instance.
   */
  public function loadJSONUrl($item_url) {

    $response = drupal_http_request($item_url, empty($this->httpOptions) ? array() : $this->httpOptions);

    $retries = 0;
    while (200 !== (int)$response->code && 5 > $retries++){
      sleep(5);
      echo("Retrying [ url :: $item_url ][ response code :: $response->code ][ retries :: $retries ]\n");
      $response = drupal_http_request($item_url, empty($this->httpOptions) ? array() : $this->httpOptions);
    }

    $json     = $response->data;
    return json_decode($json);
  }

  /**
   * Creates a stub entry.
   */
  public function emptyItem($id) {
    $result        = new stdClass();
    $result->id    = substr($id, 0, 35);
    $result->title = t('Access denied for %id', array('%id' => $id));
    $result->name  = $id;
    return $result;
  }
}

/**
 * Put our useful functions here.
 */
abstract class MigrateDKAN extends Migration {
  /**
   * Looks up format if exists, if not creates it.
   */
  public function createTax($name, $vocab_name, $vid) {
    if ($term = taxonomy_get_term_by_name($name, $vocab_name, $vid)) {
      $term = array_pop($term);
      return $term;
    }
    else {
      $new_term       = new stdClass();
      $new_term->name = $name;
      $new_term->vid  = $vid;
      $term           = taxonomy_term_save($new_term);
      return $term;
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
   * Gets Group Nid by title if exists.
   */
  public function getGroupNidByTitle($title) {
    $type   = 'group';
    $result = db_query("SELECT n.nid FROM {node} n WHERE n.title = :title AND n.type = :type", array(
      ":title" => $title,
      ":type"  => $type
    ));
    $nid    = $result->fetchField();
    if ($nid) {
      return $nid;
    }
    else {
      return NULL;
    }
  }

  /**
   * Looks up nid for resource.
   */
  public function getResourceId($uuid) {
    if ($nid = entity_get_id_by_uuid('node', array($uuid))) {
      return $nid[$uuid];
    }
  }

  /**
   * Looks up nid for group.
   */
  public function getGroupId($uuid) {
    if ($nid = entity_get_id_by_uuid('node', array($uuid))) {
      return $nid[$uuid];
    }
  }

  /**
   * Looks up nid for organization.
   */
  public function getOrganizationId($uuid) {
    if ($nid = entity_get_id_by_uuid('node', array($uuid))) {
      return $nid[$uuid];
    }
  }

  /**
   * Looks up user if they exist, if not creates them.
   *
   * @param string $creator_user_id
   *   UUID for user from CKAN.
   *
   * @return string
   *   Drupal user_id (uid).
   */
  public function getUser($creator_user_id) {
    if (isset($creator_user_id) && $creator_user_id) {
      if ($uid = entity_get_id_by_uuid('user', array($creator_user_id))) {
        return $uid[$creator_user_id];
      }
      else {
        $new_user = array(
          'uuid' => $creator_user_id,
        );
        // Get User name from CKAN API.
        $response = drupal_http_request($this->endpoint . 'user_show?id=' . $creator_user_id);
        $json     = $response->data;
        $data     = drupal_json_decode($json);
        if ($name = $data['result']['name']) {
          $current_uid = db_query("SELECT uid from {users} WHERE name = :name", array(":name" => $name))->fetchField();
          if ($current_uid) {
            $account = user_load($current_uid);
            return $account->uid;
          }
          else {
            $new_user['name'] = $name;
            $account          = user_save(NULL, $new_user);
            return $account->uid;
          }
        }
        else {
          return 1;
        }
      }
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

    $url_info      = parse_url($url);
    $url_path_info = pathinfo($url_info['path']);

    // This helps with filenames with spaces.
    $url = $url_info['scheme'] . '://' . $url_info['host'] . $url_path_info['dirname'] . '/' . rawurlencode($url_path_info['basename']);

    // Need to remove the filename from the uri.
    $uri_target    = file_uri_target($uri);
    $uri_scheme    = file_uri_scheme($uri);
    $uri_path_info = pathinfo($uri_target);
    $directory     = file_stream_wrapper_uri_normalize($uri_scheme . "://" . $uri_path_info['dirname']);

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

abstract class MigrateCkanBase extends MigrateDKAN {
  /**
   * Here we go.
   */
  public function __construct($arguments) {
    $this->endpoint = isset($arguments['endpoint']) ? $arguments['endpoint'] : 'http://demo.getdkan.com/api/3/action/';
    parent::__construct($arguments);
  }
}
