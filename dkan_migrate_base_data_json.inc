<?php

/**
 * @file
 * Migration Class for Datasets.
 */

/**
 * Migrate Data.json datasets.
 */
class MigrateDataJsonDatasetBase extends MigrateDKAN {
  /**
   * Here we go.
   */
  public function __construct($arguments) {
    parent::__construct($arguments);
    $this->endpoint = isset($arguments['endpoint']) ? $arguments['endpoint'] : 'http://demo.getdkan.com/data.json';
    $this->version = isset($arguments['version']) ? $arguments['version'] : '1.1';
    $this->page = isset($arguments['page']) ? $arguments['page'] : '';
    $this->offset = isset($arguments['offset']) ? $arguments['offset'] : '';
    $fields = $this->getDataJsonDatasetFields();

    $this->highwaterField = array('name' => 'modified');
    $this->source = new MigrateSourceList(new DataJsonListJSON(
      $this->endpoint,
      array('page' => $this->page,
            'offset' => $this->offset),
      $this->version),
      new DataJsonItemJSON($this->endpoint, array(), $this->version), $fields);

    $this->map = new MigrateSQLMap(
      $this->machineName,
      array(
        'uuid' => array(
          'type' => 'varchar',
          'length' => 255,
          'not null' => TRUE,
          'description' => 'id',
        ),
      ),
      MigrateDestinationNode::getKeySchema()
    );

    $this->addDataJsonMapping();

    $this->destination = new MigrateDestinationNode('dataset', array('text_format' => 'html'));
  }

  public function getMapping($version = '1.1') {
    $odfeFields = array();
    $baseFields = array(
      // These are common to both 1.0 and 1.1
      'title' => 'title',
      'body' => 'description',
      'og_group_ref' => 'group_id',
      'field_tags' => 'keyword',
      'changed' => 'modified',
      'created' => 'issued',
      'field_public_access_level' => 'accessLevel',
      'field_resources' => 'resources',
      'field_contact_name' => 'contactPointName',
      'field_contact_email' => 'mbox',
      'uuid' => 'identifier',
      'field_license' => 'license',
      'field_spatial_geographical_cover' => 'spatial',
      'field_temporal_coverage' => 'temporalBegin',
      'field_temporal_coverage:to' => 'temporalEnd',
      'field_frequency' => 'accrualPeriodicity',
      'field_data_dictionary' => 'describedBy',
    );
    if ($version == '1.0') {
      $addFields = array(
        'field_data_dictionary' => 'dataDictionary',
        'language' => 'language',
      );
    }
    else {
      $addFields = array(
        // POD 1.1.
        'field_additional_info' => 'field_additional_info_key',
        'field_additional_info:second' => 'field_additional_info_value',
        'field_related_content' => 'references',
      );
    }
    if (module_exists('open_data_federal_extras')) {
      $odfeFields =  array(
        'field_odfe_bureau_code' => 'bureauCode',
        'field_odfe_program_code' => 'programCode',
        'field_odfe_landing_page' => 'landingPage',
        'field_odfe_rights' => 'rights',
        'field_odfe_conforms_to' => 'conformsTo',
        'field_odfe_data_quality' => 'dataQuality',
        'field_odfe_data_dictionary_type' => 'describedByType',
        'field_odfe_is_part_of' => 'isPartOf',
        'field_odfe_language' => 'language',
        'field_odfe_investment_uii' => 'primaryITInvestmentUII',
        'field_odfe_system_of_records' => 'systemOfRecords',
        'field_odfe_category' => 'theme',
      );
    }
    return $baseFields + $addFields + $odfeFields;
  }

  /**
   * Adds Data.json mappings.
   */
  public function addDataJsonMapping($version = '1.1') {
    $fields = $this->getMapping($version);
    foreach ($fields as $dkan => $pod) {
      $this->addFieldMapping($dkan, $pod, FALSE);
    }
  }

  /**
   * Creates list of fields for Data.json Dataset.
   */
  public function getDataJsonDatasetFields($version = '1.1') {
    if ($version == '1.0') {
      return array(
        // “Common Core” Required Fields.
        "title" => "Title",
        "description" => "Description",
        "keyword" => "Tags",
        "modified" => "Last Update",
        "publisher" => "Publisher",
        "contactPoint" => "Contact Name",
        "mbox" => "Contact Email",
        "identifier" => "Unique Identifier",
        "accessLevel" => "Public",
        // “Common Core” Required-if-Applicable Fields.
        "bureauCode" => " Bureau Code",
        "programCode" => "Program",
        "accessLevelComment" => " Access Level Comment",
        "accessURL" => "Download",
        "webService" => " Endpoint ",
        "format" => " Format",
        "license" => "License",
        "spatial" => "Spatial",
        "temporal" => " Temporal",
        // Beyond Common Core.
        "theme" => "Category",
        "dataDictionary" => "Data Dictionary",
        "dataQuality" => "Data Quality",
        "distribution" => "Distribution",
        "accrualPeriodicity" => "Frequency",
        "landingPage" => "Homepage",
        "language" => "Language",
        "PrimaryITInvestmentUII" => "Primary IT Investment",
        "references" => "Related Documents",
        "issued" => "Release Date",
        "systemOfRecords" => "System of Records",
        // This doesn't actually exist but we are adding it later in prepareRow.
        "uid" => "User ID",
      );
    }
    else {
      return array(
        "title" => "Title",
        "description" => "Description",
        "keyword" => "Tags",
        "modified" => "Last Update ",
        "publisher" => "Publisher",
        "contactPoint" => " Contact Name and Email",
        "identifier" => " Unique Identifier",
        "accessLevel" => "Public",
        "bureauCode" => "Bureau",
        "programCode" => " Program Code ",
        "license" => "License",
        "rights" => " Rights",
        "spatial" => "Spatial",
        "temporal" => " Temporal",
        "distribution" => " Distribution",
        "accrualPeriodicity" => " Frequency",
        "conformsTo" => " Data Standard URI",
        "dataQuality" => " Data Quality",
        "describedBy" => "Data Dictionary",
        "describedByType" => "Data",
        "isPartOf" => " Collection",
        "issued" => " Release Date",
        "language" => " Language",
        "landingPage" => "Homepage",
        "primaryITInvestmentUII" => "Primary",
        "references" => " Related Documents ",
        "systemOfRecords" => " System of Records",
        "theme" => "Theme",
        // This doesn't actually exist but we are adding it later in prepareRow.
        "uid" => "User ID",
      );
    }
  }

  /**
   * Implements prepareRow.
   */
  public function prepareRow($row) {
    $existing = db_query(
      'SELECT nid FROM {node} WHERE title = :title',
      array(':title' => $row->title)
    )->fetchField();
    if (is_numeric($existing)) {
      $mapped = db_query(
        'SELECT destid1 FROM {migrate_map_'. $this->machineName. '} WHERE sourceid1 = :uuid',
        array(':uuid' => $row->uuid)
      )->fetchField();
      // Only update existing if it isn't already mapped.
      if (!$mapped) {
        $row->existing = node_load($existing);
      }
    }
    if (isset($row->temporal)) {
      $date = dkan_migrate_base_iso_interval_to_timestamp($row->temporal);
      if (isset($date['from'])) {
        $row->temporalBegin = $date['from'];
      }
      if (isset($date['to'])) {
        $row->temporalEnd = $date['to'];
      }
    }
    if (isset($row->keyword)) {
      $tags = taxonomy_vocabulary_machine_name_load('tags');
      foreach ($row->keyword as $tag) {
        $this->createTax($tag, 'tags', $tags->vid);
      }
    }
    if (isset($row->theme) && module_exists('open_data_federal_extras')) {
      $category = taxonomy_vocabulary_machine_name_load('odfe_theme');
      foreach ($row->theme as $cat) {
        $this->createTax($cat, 'odfe_theme', $category->vid);
      }
    }
    if ($this->version == '1.0') {
    }
    else {
      // POD 1.1.
      if (isset($row->publisher->name) && !$row->group_id = $this->getGroupNidByTitle($row->publisher->name)) {
        if ($row->publisher->name) {
          $group = new stdClass;
          $group->title = $row->publisher->name;
          $group = $this->createGroupNode($group);
          $this->map->saveIDMapping($group, array($group->nid), 0, 0, '');
          $row->group_id = $group->nid;
        }
      }
      if (isset($row->theme) && is_array($row->theme)) {
        $category = taxonomy_vocabulary_machine_name_load('odfe_theme');
        foreach ($row->theme as $theme) {
          $this->createTax($theme, 'odfe_theme', $category->vid);
        }
      }
      if (is_array($row->contactPoint)
        && $row->contactPoint['hasEmail']) {
        $row->mbox = str_replace("mailto:", "", $row->contactPoint['hasEmail']) ? str_replace("mailto:", "", $row->contactPoint['hasEmail']) : $row->contactPoint['hasEmail'];
        $row->contactPointName = isset($row->contactPoint['fn']) ? $row->contactPoint['fn'] : '';
        // Must be a valid email address.
        if (!filter_var($row->mbox, FILTER_VALIDATE_EMAIL)) {
          $row->mbox = '';
        }
      }
      elseif (is_object($row->contactPoint)
        && $row->contactPoint->hasEmail) {
        $row->mbox = str_replace("mailto:", "", $row->contactPoint->hasEmail) ? str_replace("mailto:", "", $row->contactPoint->hasEmail) : $row->contactPoint->hasEmail;
        $row->contactPointName = isset($row->contactPoint->fn) ? $row->contactPoint->fn : '';
        if (!filter_var($row->mbox, FILTER_VALIDATE_EMAIL)) {
          $row->mbox = '';
        }
      }
      if (isset($row->dataQuality)) {
        if ($row->dataQuality === FALSE) {
          $row->dataQuality = 'false';
        }
        elseif ($row->dataQuality === TRUE) {
          $row->dataQuality = 'true';
        }
      }

    }
    $row->resources = $this->prepareResources($row);
    $this->prepareAdditionalInfo($row);
    $row->uid = 1;
  }

  public function prepareAdditionalInfo($row) {
    $ignore = array(
      '@type',
      'distribution',
      'uuid',
      'tag_names',
      'group_id',
      'modified',
      'mbox',
      'contactPointName',
      'temporalBegin',
      'temporalEnd',
      'publisher-name',
      'existing',
      'migrate_map_sourceid1',
      'migrate_map_destid1',
      'migrate_map_needs_update',
      'migrate_map_rollback_action',
      'migrate_map_last_imported',
      'migrate_map_hash',
    );
    $mapped_fields = array_keys($this->getDataJsonDatasetFields());
    $row_keys = (array) $row;
    $row_keys = array_keys($row_keys);
    foreach ($row_keys as $key) {
      if (!in_array($key, $mapped_fields) && !in_array($key, $ignore)) {
        if (is_object($row->{$key}) || is_array($row->$key)) {
          foreach ((array) $row->{$key} as $k => $v) {
            $k = $key . '-' . $k;
            $row->field_additional_info_key[] = substr($k, 0, 254);
            $row->field_additional_info_value[] = substr($v, 0, 254);
          }
        }
        else {
          $row->field_additional_info_key[] = substr($key, 0, 254);
          $row->field_additional_info_value[] = substr($row->{$key}, 0, 254);
        }
      }
    }
  }

  /**
   * Prepare Resources.
   */
  public function prepareResources($row) {
    $resources = array();
    if ($row->existing && isset($row->existing->field_resources[LANGUAGE_NONE])) {
      foreach ($row->existing->field_resources[LANGUAGE_NONE] as $resource) {
        $resource = node_load($resource['target_id']);
        // TODO: updateResourceNode($nid);
        // 1. pre-check title
        // 2. Loads existing node
        // 3. Updates values.
        // 4. Removes unused from mapping?
        //
        if (isset($resource->field_link_api[LANGUAGE_NONE]) && count($resource->field_link_api[LANGUAGE_NONE])) {
          $url = $resource->field_link_api[LANGUAGE_NONE][0]['url'];
          foreach ($row->distribution as $key => $new_resource) {
            if ($new_resource['downloadURL'] == $url) {
              unset($row->distribution[$key]);
              $resources[] = $resource->nid;
              $this->map->saveIDMapping($resource, array($resource->nid), 0, 0, '');
            }
          }
        }
      }
    }
    if (isset($row->distribution)) {
      foreach ($row->distribution as $resource) {
        if (!is_array($resource)) {
          $resource = (array) $resource;
        }
        $res = new stdClass();
        $res->name = isset($resource['name']) ? $resource['name'] : $row->title;
        // Provide default for distribution items with absent name property.
        if (isset($resource['mediaType']) && $res->name == $row->title) {
          $res->format = recline_get_data_type($resource['mediaType']);
          $res->name = $res->name . ' - ' . $res->format;
        }
        $res->url = $resource['downloadURL'];
        $resource = $this->createResourceNode($res);
        $resources[] = $resource->nid;
        $this->map->saveIDMapping($resource, array($resource->nid), 0, 0, '');
      }
    }
    return $resources;
  }

  /**
   * Implements prepareRow.
   */
  public function prepare($entity, $row) {
    // Remove empy additional fields.
    if (isset($entity->field_additional_info['und'][0]) && $entity->field_additional_info['und'][0]['first'] == NULL) {
      unset($entity->field_additional_info);
    }
    if (isset($row->existing)) {
      $entity->nid = $row->existing->nid;
    }
    if (isset($entity->field_temporal_coverage['und'][0]['value'])) {
      $entity->field_temporal_coverage['und'][0]['timezone'] = 'UTC';
    }
    $entity->status = 1;
    $entity->vid = isset($entity->vid) ? $entity->vid : '';
  }
}

class DataJsonListJSON extends MigrateListJSON {

  /**
   * Adds version to class.
   */
  public function __construct($list_url, $http_options = array(), $version = '1.0') {
    parent::__construct($list_url);
    $this->listUrl = $list_url;
    $this->httpOptions = $http_options;
    $this->page = isset($http_options['page']) ? $http_options['page'] : '';
    $this->offset = isset($http_options['offset']) ? $http_options['offset'] : '';
    $this->version = $version;
  }

  /**
   * Implements computeCount().
   */
  public function computeCount() {
    $count = 0;
    if (!filter_var($this->listUrl, FILTER_VALIDATE_URL)) {
      $json = file_get_contents($this->listUrl);
    }
    else {
      $response = drupal_http_request($this->listUrl, $this->httpOptions);
      $json = $response->data;
    }
    if ($json) {
      $data = drupal_json_decode($json);
      if ($this->version == '1.1') {
        $datasets = 0;
        if (isset($data)) {
          foreach ($data['dataset'] as $item) {
            // Adds resources to count.
            if (isset($item['distribution'])) {
              $count++;
              $count = $count + count($item['distribution']);
            }
            if (isset($item['publisher']['name'])) {
              $count++;
            }
            $datasets++;
            if ($this->page && $datasets >= $this->page) {
              break;
            }
          }
        }
      }
      else {
        foreach ($data as $item) {
          // Adds resources to count.
          if (isset($item['distribution'])) {
            $count++;
            $count = $count + count($item['distribution']);
          }
        }
      }
    }
    if ($this->version == '1.1') {
      $dataset_count = $this->page ? $this->page : count($data['dataset']);
    }
    else {
      $dataset_count = $this->page ? $this->page : count($data);
    }
    $resource_count = $count - $dataset_count;
    drupal_set_message(t("%url contains %datasets datasets and %resources resources.", array('%datasets' => $dataset_count, '%url' => $this->listUrl, '%resources' => $resource_count )));
    return $count;
  }

  public function getIdList() {
    migrate_instrument_start("Retrieve $this->listUrl");
    if (!filter_var($this->listUrl, FILTER_VALIDATE_URL)) {
      $json = file_get_contents($this->listUrl);
    }
    else {
      $response = drupal_http_request($this->listUrl, $this->httpOptions);
      $json = $response->data;
    }
    migrate_instrument_stop("Retrieve $this->listUrl");
    if ($json) {
      $data = drupal_json_decode($json);
      if ($data) {
        return $this->getIDsFromJSON($data);
      }
    }
    Migration::displayMessage(t('Loading of !listurl failed:',
        array('!listurl' => $this->listUrl)));
    return NULL;
  }

  /**
   * Retrieves ID from list.
   */
  protected function getIDsFromJSON(array $data) {
    $ids = array();
    if ($this->version == '1.0') {
      foreach ($data as $item) {
        $ids[] = $item['identifier'];
      }
    }
    else {
      $datasets = 0;
      $total = $this->page + $this->offset;
      foreach ($data['dataset'] as $item) {
        if ($datasets < $this->offset) {
          $datasets++;
          continue;
        }
        $uuid = explode("\\", $item['identifier']);
        $ids[] = array_pop($uuid);
        $datasets++;
        if ($total && $datasets >= $total) {
          break;
        }
      }
    }
    return $ids;
  }
}

class DataJsonItemJSON extends MigrateItemJSON {
  protected $data = array();
  /**
   * Adds version to class.
   */
  public function __construct($list_url, $http_options = array(), $version = '1.0') {
    parent::__construct($list_url);
    $this->listUrl = $list_url;
    $this->httpOptions = $http_options;
    $this->version = $version;
  }

  /**
   * Retrieves from itemUrl.
   */
  public function getItem($id) {
    // We cache the parsed JSON at $this->data.
    if (empty($this->data)) {
      $data = $this->loadJSONUrl($this->itemUrl);
      if ($data) {
        if ($this->version == '1.0') {
          foreach ($data as $item) {
            $this->data[$item->identifier] = $item;
          }
        }
        else {
          foreach ($data->dataset as $item) {
            $this->data[$item->identifier] = $item;
          }
        }
      }
      else {
        // TODO: Proper error handling.
      }
    }
    // Return the requested item.
    if (isset($this->data[$id])) {
      return $this->data[$id];
    }
    else {
      return NULL;
    }
  }
}
