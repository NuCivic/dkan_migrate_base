<?php

/**
 * @file
 * Migration Class for Datasets.
 */

/**
 * Migrate Data.json datasets.
 */
class MigrateDataJsonDatasetBase extends Migration {
  /**
   * Here we go.
   */
  public function __construct($arguments) {
    parent::__construct($arguments);
    $this->endpoint = isset($arguments['endpoint']) ? $arguments['endpoint'] : 'http://example.com/data.json';
    $this->version = '1.0';
    $fields = $this->getDataJsonDatasetFields();

    $this->highwaterField = array(
      'name' => 'modified',
    );
    $this->source = new MigrateSourceList(new DataJsonListJSON($this->endpoint, array(), $this->version),
      new DataJsonItemJSON($this->endpoint, array()), $fields);

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

    if ($this->version == '1.0') {
      $this->addFieldMapping('title', 'title');
      $this->addFieldMapping('body', 'description');
      $this->addFieldMapping('changed', 'modified');
      $this->addFieldMapping('field_publisher', 'publisher');
      $this->addFieldMapping('field_contact_name', 'contactPoint');
      $this->addFieldMapping('field_contact_email', 'mbox');
      $this->addFieldMapping('uuid', 'identifier');
      $this->addFieldMapping('field_public_access_level', 'accessLevel');
      $this->addFieldMapping('field_additional_info', 'webService');
      $this->addFieldMapping('field_license', 'license');
      $this->addFieldMapping('field_spatial_geographical_cover', 'spatial');
      $this->addFieldMapping('field_temporal', 'temporal');
      $this->addFieldMapping('field_data_dictionary', 'dataDictionary');
      $this->addFieldMapping('field_frequency', 'accrualPeriodicity');
      $this->addFieldMapping('language', 'language');
      $this->addFieldMapping('created', 'issued');

      // We process keywords in prepareRow to make sure taxonomy exists.
      $this->addFieldMapping('field_tags', 'tag_names');
      // $this->addFieldMapping('field_tags', 'keyword');

      // We will add resources separately.
      // $this->addFieldMapping('field_resources', 'distribution');

      // Part of Open Data Federal Extras module.
      // $this->addFieldMapping('', 'bureauCode');
      // $this->addFieldMapping('', 'programCode');

      // Not implemented.
      // $this->addFieldMapping('', 'accessLevelComment');
      // $this->addFieldMapping('', 'theme');
      // $this->addFieldMapping('', 'dataQuality');
      // $this->addFieldMapping('', 'PrimaryITInvestmentUII');
      // $this->addFieldMapping('', 'landingPage');
      // $this->addFieldMapping('', 'systemOfRecords');

      // For resource if only one file.
      // $this->addFieldMapping('', 'accessURL');
      // $this->addFieldMapping('', 'format');
    }
    else {
      // TODO: Add POD 1.1.
    }

    $this->destination = new MigrateDestinationNode('dataset');
  }

  /**
   * Creates list of fields for Data.json Dataset.
   */
  public function getDataJsonDatasetFields($version = '1.0') {
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
        "bureauCodeUSG" => "Bureau",
        "programCodeUSG" => " Program Code ",
        "license" => "License",
        "rights" => " Rights",
        "spatial" => "Spatial",
        "temporal" => " Temporal",
        "distribution" => " Distribution",
        "accrualPeriodicity" => " Frequency",
        "conformsTo" => " Data Standard URI",
        "dataQualityUSG" => " Data Quality",
        "describedBy" => "Data",
        "describedByType" => "Data",
        "isPartOf" => " Collection",
        "issued" => " Release Date",
        "language" => " Language",
        "landingPage" => "Homepage",
        "primaryITInvestmentUIIUSG" => "Primary",
        "references" => " Related Documents ",
        "systemOfRecordsUSG" => " System of Records",
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
    if ($this->version == '1.0') {
      $tags = taxonomy_vocabulary_machine_name_load('tags');
      if (isset($row->keyword)) {
        foreach ($row->keyword as $tag) {
          $this->createTax($tag, 'tags', $tags->vid);
          $row->tag_names[] = $tag;
        }
      }
    }
    else {
      // TODO: POD 1.1.
    }
  }

  /**
   * Implements prepareRow.
   */
  public function prepare($entity, $row) {
    if ($this->version == '1.0') {
      $resource_nids = array();
      if ($row->distribution) {
        foreach ($row->distribution as $key => $dist) {
          // Creates a new resource for every linked file.
          // Linked files contain title, format, and accessURL.
          global $user;
          $node = new stdClass();
          $node->is_new = TRUE;
          $node->language = 'und';
          $node->uid = $user->uid;
          $node->type = 'resource';
          $node->title = $dist->title;
          $dist->format = strtolower($dist->format);
          // TODO: Use this->createTax() to create new tags.
          $node->field_format = $dist->format;
          $file = remote_stream_wrapper_file_load_by_uri($dist->accessURL);
          // Create a link to file for certain formats.
          if ($dist->format == 'csv' || $dist->format == 'zip') {
            if (!$file) {
              $file = remote_stream_wrapper_file_create_by_uri($dist->accessURL);
              $file->status = FALSE;
              file_save($file);
            }
            $node->field_link_remote_file['und'][0]['fid'] = $file->fid;
            $node->field_link_remote_file['und'][0]['display'] = 1;
          }
          else {
            // Link to API for all others.
            $node->field_link_api['und'][0]['url'] = $dist->accessURL;
          }
          node_save($node);
          $resource_nids[] = $node->nid;
          // Adds the node to the map. This means that when the migration is
          // rolled back that the resources are removed as well. This adds the
          // newly created uuids and nids to the migrate_map_data_json table.
          $this->map->saveIDMapping($node, array($node->nid), 0, 0, '');
        }
      }
      // Link the new resources to the dataset.
      foreach ($resource_nids as $num => $nid) {
        $entity->field_resources['und'][$num]['target_id'] = $nid;
      }
    }
    else {
      // TODO: POD 1.1.
    }
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
}

class DataJsonListJSON extends MigrateListJSON {

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
      foreach ($data as $item) {
        // Adds resources to count.
        if (isset($item['distribution'])) {
          $count++;
          $count = $count + count($item['distribution']);
        }
      }
    }
    return $count;
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
      foreach ($data['dataset'] as $item) {
        $ids[] = $item['identifier'];
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
          foreach ($data['dataset'] as $item) {
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
